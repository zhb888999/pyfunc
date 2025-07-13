import numpy as np
from typing import Tuple

from cpp_entry import cpp_entry


@cpp_entry
def generate_array(shape, dtype):
    print(shape)
    return np.random.random(shape).astype(dtype)

@cpp_entry
def read_array(array1, array2):
    print(np.sum(array1 - array2))
    return

@cpp_entry
def test_empty_args(b=1):
    print("test_empty_args")
    return