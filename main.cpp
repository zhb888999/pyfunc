#include "pyfunc.h"

int main() {
  PyFunc generate_array("main", "generate_array");
  std::string dtype = "float32";
  std::vector<int> shape = {10, 1000, 1000};
  generate_array.call(
    shape, dtype
  );
  auto array0 = generate_array.get<NDArray>();
  generate_array.reset();
  std::cout << "is array:" << generate_array.is<NDArray>()
            << std::endl;
  auto array1 = generate_array.get<NDArray>();

  std::map<std::string, NDArray> map =
      {{"array0", std::move(array0)},
       {"array1", std::move(array1)}};
  PyFunc test("main", "read_array");
  test.call(map);
  auto output = test.gets<float, bool>();
  std::cout << "sum value:" << std::get<0>(output)
            << " is equal:" << std::get<1>(output)
            << std::endl;
  return 0;
}
