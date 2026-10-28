#include <iostream>

void print_array(const int numbers[], int size) {
    for (int i = 0; i < size; i++) {
        std::cout << numbers[i] << " ";
    }
    std::cout << std::endl;
}

void bubble_sort(int numbers[], int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (numbers[j] > numbers[j + 1]) {
                int temporary = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temporary;
            }
        }
    }
}

int main() {
    int numbers[] = {7, 2, 9, 1, 5, 3, 8};
    int size = 7;

    std::cout << "Before sorting: ";
    print_array(numbers, size);

    bubble_sort(numbers, size);

    std::cout << "After sorting:  ";
    print_array(numbers, size);

    return 0;
}

