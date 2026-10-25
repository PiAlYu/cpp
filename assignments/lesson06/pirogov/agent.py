import argparse
import json
import os
import shlex
import shutil
import subprocess

from pathlib import Path
from dotenv import load_dotenv
from openai import OpenAI


parser = argparse.ArgumentParser()
parser.add_argument("--project", default=".")
args = parser.parse_args()

load_dotenv(Path(__file__).with_name(".env"))

PROJECT_ROOT = Path(args.project).resolve()
API_KEY = os.environ["OPENAI_API_KEY"]
BASE_URL = os.environ["OPENAI_BASE_URL"]
MODEL = os.environ.get("OPENAI_MODEL", "qwen38_27b")

client = OpenAI(api_key=API_KEY, base_url=BASE_URL)

FORBIDDEN_PARTS = {".git", ".venv"}
FORBIDDEN_FILES = {".env"}
FORBIDDEN_TOKENS = {";", "&&", "||", "|", ">", "<", "$()"}


def safe_path(relative_path):
    path = (PROJECT_ROOT / relative_path).resolve()
    if path != PROJECT_ROOT and PROJECT_ROOT not in path.parents:
        raise ValueError("path is outside the project")

    relative = path.relative_to(PROJECT_ROOT)
    if any(part in FORBIDDEN_PARTS for part in relative.parts):
        raise ValueError("access to this directory is forbidden")
    if relative.name in FORBIDDEN_FILES:
        raise ValueError("access to this file is forbidden")
    return path


def list_files():
    files = []
    for path in PROJECT_ROOT.rglob("*"):
        relative = path.relative_to(PROJECT_ROOT)
        if any(part in FORBIDDEN_PARTS or part == "build" for part in relative.parts):
            continue
        if path.is_file():
            files.append(str(relative).replace("\\", "/"))
    files.sort()
    return "\n".join(files[:200])


def read_file(path):
    try:
        file_path = safe_path(path)
        if not file_path.is_file():
            return f"ERROR: file not found: {path}"
        text = file_path.read_text(encoding="utf-8")
        return text[:30000]
    except Exception as error:
        return f"ERROR: cannot read file: {error}"


def write_file(path, content):
    try:
        relative = Path(path)
        if relative.is_absolute():
            return "ERROR: write_file accepts only a relative path"
        if "build" in relative.parts:
            return "ERROR: writing generated files inside build is forbidden"

        file_path = safe_path(path)
        if file_path.is_dir():
            return f"ERROR: path is a directory: {path}"
        if file_path.suffix in {".cpp", ".hpp"}:
            if "#include" not in content or content.count("{") != content.count("}"):
                return (
                    "ERROR: refusing to write an incomplete C++ file. "
                    "Keep includes and balanced braces."
                )
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text(content, encoding="utf-8")
        return f"OK: file written: {path}"
    except Exception as error:
        return f"ERROR: cannot write file: {error}"


def find_program(name):
    found = shutil.which(name)
    if found:
        return found

    visual_studio = Path(
        "C:/Program Files/Microsoft Visual Studio/18/Community/"
        "Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin"
    )
    candidate = visual_studio / f"{name}.exe"
    if candidate.exists():
        return str(candidate)
    return None


def run_command(command):
    if any(token in command for token in FORBIDDEN_TOKENS):
        return "ERROR: shell operators are not allowed"

    try:
        command_args = shlex.split(command, posix=os.name != "nt")
    except ValueError as error:
        return f"ERROR: invalid command: {error}"

    if not command_args:
        return "ERROR: empty command"

    program = command_args[0]
    executable = None

    if program in {"cmake", "ctest"}:
        executable = find_program(program)
        if not executable:
            return f"ERROR: program not found: {program}"

        if program == "cmake" and "--build" not in command_args:
            if "-S" not in command_args or "-B" not in command_args:
                return "ERROR: configure CMake only with: cmake -S . -B build"
            build_index = command_args.index("-B") + 1
            if build_index >= len(command_args) or command_args[build_index] != "build":
                return "ERROR: CMake build directory must be build"

        if program == "cmake" and "--build" in command_args:
            build_index = command_args.index("--build") + 1
            if build_index >= len(command_args) or command_args[build_index] != "build":
                return "ERROR: build only the build directory"
    elif program.startswith("build/") or program.startswith("build\\"):
        try:
            executable_path = safe_path(program)
        except ValueError as error:
            return f"ERROR: {error}"

        build_root = (PROJECT_ROOT / "build").resolve()
        if build_root not in executable_path.parents:
            return "ERROR: executable must be inside build/"
        if not executable_path.is_file():
            return f"ERROR: executable not found: {program}"
        executable = str(executable_path)
    else:
        return f"ERROR: command is not allowed: {program}"

    try:
        result = subprocess.run(
            [executable, *command_args[1:]],
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=60,
            errors="replace",
        )
    except subprocess.TimeoutExpired:
        return "ERROR: command timed out after 60 seconds"
    except Exception as error:
        return f"ERROR: command failed: {error}"

    return (
        f"exit_code={result.returncode}\n"
        f"stdout:\n{result.stdout}\n"
        f"stderr:\n{result.stderr}"
    )[:30000]


tools = [
    {
        "type": "function",
        "function": {
            "name": "list_files",
            "description": "List files inside the project.",
            "parameters": {"type": "object", "properties": {}},
        },
    },
    {
        "type": "function",
        "function": {
            "name": "read_file",
            "description": "Read a UTF-8 text file inside the project.",
            "parameters": {
                "type": "object",
                "properties": {"path": {"type": "string"}},
                "required": ["path"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "write_file",
            "description": "Write complete UTF-8 contents to a file inside the project.",
            "parameters": {
                "type": "object",
                "properties": {
                    "path": {"type": "string"},
                    "content": {"type": "string"},
                },
                "required": ["path", "content"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "run_command",
            "description": (
                "Run one command. Allowed commands: cmake, ctest, "
                "or an executable located inside build/."
            ),
            "parameters": {
                "type": "object",
                "properties": {"command": {"type": "string"}},
                "required": ["command"],
            },
        },
    },
]


user_prompt = input("Что нужно сделать? ").strip()
if not user_prompt:
    raise RuntimeError("Задача не задана")

messages = [
    {
        "role": "system",
        "content": (
            "You are a simple coding agent. Inspect the project with tools. "
            "Use one tool at a time. Do not invent command results. "
            "Change files only when the user asks. Always verify changes. "
            "For a build task, first list and read project files. "
            "Configure only with 'cmake -S . -B build'. "
            "Build on Windows with 'cmake --build build --config Debug'. "
            "A successful configure is not a successful build. "
            "Do not report success until the build exits with code 0."
        ),
    },
    {
        "role": "user",
        "content": user_prompt + " Start by calling list_files.",
    },
]

prompt_lower = user_prompt.lower()
needs_build = "build" in prompt_lower
needs_run = "run the program" in prompt_lower
needs_test = "run tests" in prompt_lower or "execute tests" in prompt_lower
needs_inspection = "study" in prompt_lower or "review" in prompt_lower
successful_build = False
successful_program = False
successful_tests = False
configured = False
read_paths = set()
available_files = set()
last_build_failed = False
changed_after_failure = False

for step in range(20):
    print(f"\n=== STEP {step + 1} ===")

    forced_tool = None
    if (needs_build or needs_inspection) and not available_files:
        forced_tool = "list_files"
    elif needs_inspection:
        important_files = [
            "CMakeLists.txt",
            "README.md",
            "include/chat.hpp",
            "src/chat.cpp",
            "src/config.cpp",
            "src/main.cpp",
            "src/http_client.cpp",
            "tests/chat_test.cpp",
        ]
        unread_files = [
            path for path in important_files
            if path in available_files and path not in read_paths
        ]
        if unread_files:
            forced_tool = "read_file"
    elif needs_build:
        if "CMakeLists.txt" not in read_paths:
            forced_tool = "read_file"
        elif "main.cpp" not in read_paths and "src/main.cpp" not in read_paths:
            forced_tool = "read_file"
        elif not configured:
            forced_tool = "run_command"
        elif not successful_build:
            if last_build_failed and not changed_after_failure:
                forced_tool = "write_file"
            else:
                forced_tool = "run_command"
        elif needs_test and not successful_tests:
            forced_tool = "run_command"
        elif needs_run and not successful_program:
            forced_tool = "run_command"

    tool_choice = "auto"
    if forced_tool:
        tool_choice = {
            "type": "function",
            "function": {"name": forced_tool},
        }
    if forced_tool == "write_file":
        source_path = "main.cpp"
        if "main.cpp" not in read_paths and "src/main.cpp" in read_paths:
            source_path = "src/main.cpp"
        source_text = read_file(source_path)
        include_lines = "\n".join(
            line for line in source_text.splitlines()
            if line.strip().startswith("#include")
        )
        messages.append(
            {
                "role": "user",
                "content": (
                    "The compiler error is already known. Call write_file now. "
                    f"The path must be '{source_path}'. Return every line of the "
                    "complete source, not only the changed line. Fix the reported "
                    "compiler error. If a semicolon is missing before return, add "
                    "it to the previous statement. Keep these include lines at "
                    f"the start:\n{include_lines}\nDo not read the file again.\n\n"
                    f"CURRENT SOURCE:\n{source_text}"
                ),
            }
        )

    response = client.chat.completions.create(
        model=MODEL,
        messages=messages,
        tools=tools,
        tool_choice=tool_choice,
        temperature=0.0,
        max_tokens=512,
    )
    message = response.choices[0].message
    messages.append(message)

    if not message.tool_calls:
        missing = []
        inspection_files = [
            "CMakeLists.txt",
            "README.md",
            "include/chat.hpp",
            "src/chat.cpp",
            "src/config.cpp",
            "src/main.cpp",
            "src/http_client.cpp",
            "tests/chat_test.cpp",
        ]
        pending_reads = [
            path for path in inspection_files
            if path in available_files and path not in read_paths
        ]
        if needs_inspection and not available_files:
            missing.append("the project files have not been listed")
        elif needs_inspection and pending_reads:
            missing.append("the important project files have not all been read")
        if needs_build and not successful_build:
            missing.append("the project has not been built successfully")
        if needs_run and not successful_program:
            missing.append("the built program has not been run successfully")
        if needs_test and not successful_tests:
            missing.append("the tests have not passed successfully")

        if missing:
            if (needs_inspection or needs_build) and not available_files:
                next_action = "Call list_files now."
            elif needs_inspection and pending_reads:
                next_action = f"Call read_file with path {pending_reads[0]} now."
            elif "CMakeLists.txt" not in read_paths:
                next_action = "Call read_file with path CMakeLists.txt now."
            elif "main.cpp" not in read_paths and "src/main.cpp" not in read_paths:
                main_path = "src/main.cpp" if "src/main.cpp" in available_files else "main.cpp"
                next_action = f"Call read_file with path {main_path} now."
            elif not configured:
                next_action = "Call run_command with: cmake -S . -B build"
            elif not successful_build:
                if last_build_failed and not changed_after_failure:
                    next_action = (
                        "Use the compiler error. Call write_file now with the "
                        "complete corrected file. Do not read or rebuild first."
                    )
                else:
                    next_action = (
                        "Call run_command with: "
                        "cmake --build build --config Debug"
                    )
            elif needs_test and not successful_tests:
                next_action = (
                    "Call run_command with: "
                    "ctest --test-dir build -C Debug --output-on-failure"
                )
            else:
                next_action = (
                    "Call run_command with: build/Debug/demo_app.exe"
                )

            messages.append(
                {
                    "role": "user",
                    "content": (
                        "The task is not finished: " + ", ".join(missing) + ". "
                        + next_action
                    ),
                }
            )
            print("Agent tried to finish too early. Continuing.")
            continue

        print("\nFINAL RESPONSE:\n")
        print(message.content)
        break

    for tool_call in message.tool_calls:
        try:
            arguments = json.loads(tool_call.function.arguments)
        except json.JSONDecodeError as error:
            result = f"ERROR: invalid tool arguments: {error}"
        else:
            name = tool_call.function.name
            print(f"\nTOOL: {name} {arguments}\n")

            if forced_tool and name != forced_tool:
                result = f"ERROR: expected tool {forced_tool}, got {name}"
            elif name == "list_files":
                result = list_files()
                available_files = set(result.splitlines())
            elif name == "read_file":
                path = arguments.get("path", "")
                result = read_file(path)
                if not result.startswith("ERROR:"):
                    read_paths.add(path.replace("\\", "/"))
            elif name == "write_file":
                write_path = arguments.get("path", "")
                if write_path.replace("\\", "/") not in read_paths:
                    result = "ERROR: write only a source file that was read"
                else:
                    result = write_file(
                        write_path,
                        arguments.get("content", ""),
                    )
                if not result.startswith("ERROR:"):
                    changed_after_failure = True
            elif name == "run_command":
                command = arguments.get("command", "")
                if command.startswith("cmake --build") \
                        and last_build_failed and not changed_after_failure:
                    result = (
                        "ERROR: the same build already failed. "
                        "Modify the source before rebuilding."
                    )
                else:
                    result = run_command(command)
                if command.startswith("cmake -S") and "exit_code=0" in result:
                    configured = True
                if command.startswith("cmake --build") and "exit_code=0" in result:
                    successful_build = True
                    last_build_failed = False
                elif command.startswith("cmake --build"):
                    last_build_failed = True
                    changed_after_failure = False
                if (command.startswith("build/") or command.startswith("build\\")) \
                        and "exit_code=0" in result:
                    successful_program = True
                if command.startswith("ctest") and "exit_code=0" in result:
                    successful_tests = True
            else:
                result = f"ERROR: unknown tool: {name}"

        print(result)
        messages.append(
            {
                "role": "tool",
                "tool_call_id": tool_call.id,
                "content": result,
            }
        )
else:
    print("Agent stopped: maximum number of steps reached.")
