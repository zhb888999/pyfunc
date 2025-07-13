#include "pyfunc.h"

int main() {
    PyFunc generate_array("main", "generate_array");
    std::string dtype = "float32";
    std::vector<int> shape = {10, 1000, 1000};
    generate_array.call(
        shape, dtype
    );
    auto array = generate_array.get<NDArray>();

    PyFunc test("main", "read_array");
    test.call(array, array);
    auto output = test.get<None>();

    // PyFunc emt("main", "test_empty_args");
    // emt.call();
    // auto output1 = emt.get<None>();

    return 0;
}
