import numpy as np
from typing import Tuple

from cpp_entry import cpp_entry


@cpp_entry
def generate_array(shape, dtype):
    print(shape)
    return np.random.random(shape).astype(dtype)

@cpp_entry
def read_array(array_map):
    sum_value = float(np.sum(array_map["array0"] - array_map["array1"]))
    return sum_value, sum_value == 0
