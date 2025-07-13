import traceback
from abc import ABCMeta, abstractmethod
from typing import Any, List, Tuple, Union
import struct
import sys

import numpy as np


class Converter(metaclass=ABCMeta):
    @abstractmethod
    def serialize(self, value: Any) -> Union[bytes, List[bytes]]: ...

    @abstractmethod
    def deserialize(self, value: bytes) -> Any: ...

class ConverterFactory:
    def __init__(self):
        self._converters = {}
        self._converter2id = {}
        self._id2converter = {}
        self._auto_increment_id = 0

    @property
    def auto_increment_id(self):
        self._auto_increment_id += 1
        return self._auto_increment_id

    def __getitem__(self, key):
        if isinstance(key, int):
            return self._id2converter[key]()
        return self._converters[key]()

    def register(self, conv_type, typeid=-1):
        if typeid < 0:
            typeid = self.auto_increment_id
        else:
            assert typeid not in self._id2converter, f"Type ID {typeid} already exists"
        assert conv_type not in self._converters, f"Type {conv_type} already exists"

        def wrap(cls):
            self._converters[conv_type] = cls
            self._converter2id[conv_type] = typeid
            self._id2converter[self._converter2id[conv_type]] = cls
            return cls
        return wrap

    def serialize(self, *args) -> List[bytes]:
        result = []
        for arg in args:
            data = self.serialize_type(arg)
            if isinstance(data, list):
                result.extend(data)
            else:
                result.append(data)
        return result

    def serialize_type(self, value: Any) -> List[bytes]:
        data = []
        value_type = type(value)
        data.append(self._converter2id[value_type].to_bytes(4, byteorder=sys.byteorder))
        value_data = self[value_type].serialize(value)
        if isinstance(value_data, list):
            length = sum(len(item) for item in value_data)
            data.append(length.to_bytes(8, byteorder=sys.byteorder))
            data.extend(value_data)
        else:
            data.append(len(value_data).to_bytes(8, byteorder=sys.byteorder))
            data.append(value_data)
        return data


    def deserialize(self, value: bytes):
        size, offset = len(value), 0
        while True:
            if (offset >= size):
                break
            converter_id = int.from_bytes(value[offset:offset+4], byteorder=sys.byteorder)
            offset += 4
            data_size = int.from_bytes(value[offset:offset+8], byteorder=sys.byteorder)
            offset += 8
            yield self.deserialize_type(
                converter_id, value[offset:offset+data_size]
            )
            offset += data_size

    def deserialize_type(self, id, value: bytes) -> Any:
        return self[id].deserialize(value)


convert_factory = ConverterFactory()

# id 1
@convert_factory.register(type(None))
class NoneConverter(Converter):
    def serialize(self, value: int) -> bytes:
        return b''

    def deserialize(self, value: bytes) -> None:
        return

# id 2
@convert_factory.register(bool)
class BoolConverter(Converter):
    def serialize(self, value: bool) -> bytes:
        return value.to_bytes(1, byteorder=sys.byteorder)

    def deserialize(self, value: bytes) -> bool:
        return bool.from_bytes(value, byteorder=sys.byteorder)

# id 3
@convert_factory.register(int)
class IntConverter(Converter):
    def serialize(self, value: int) -> bytes:
        return value.to_bytes(8, byteorder=sys.byteorder, signed=True)

    def deserialize(self, value: bytes) -> int:
        return int.from_bytes(value, byteorder=sys.byteorder, signed=True)

# id 4
@convert_factory.register(float)
class FloatConverter(Converter):
    def serialize(self, value: float) -> bytes:
        return struct.pack('d', value)

    def deserialize(self, value: bytes) -> float:
        return struct.unpack('d', value)[0]

# id 5
@convert_factory.register(str)
class StrConverter(Converter):
    def serialize(self, value: str) -> bytes:
        return value.encode('utf-8')

    def deserialize(self, value: bytes) -> str:
        return value.decode('utf-8')

# id 6
@convert_factory.register(bytes)
class BytesConverter(Converter):
    def serialize(self, value: bytes) -> bytes:
        return value

    def deserialize(self, value: bytes) -> bytes:
        return value

# id 7
@convert_factory.register(list)
class ListConverter(Converter):
    def serialize(self, value: List[Any]) -> List[bytes]:
        data = []
        for item in value:
            data.extend(convert_factory.serialize(item))
        return data

    def deserialize(self, value: bytes) -> List[Any]:
        return [arg for arg in convert_factory.deserialize(value)]

# id 8
@convert_factory.register(tuple)
class TupleConverter(Converter):
    def serialize(self, value: Tuple[Any]) -> List[bytes]:
        data = []
        for item in value:
            data.extend(convert_factory.serialize(item))
        return data

    def deserialize(self, value: bytes) -> Tuple[Any]:
        return tuple(arg for arg in convert_factory.deserialize(value))

# id 9
@convert_factory.register(dict)
class DictConverter(Converter):
    def serialize(self, value: dict) -> List[bytes]:
        data = []
        for k, v in value.items():
            data.extend(convert_factory.serialize(k))
            data.extend(convert_factory.serialize(v))
        return data

    def deserialize(self, value: bytes) -> dict:
        result, k, v = {}, None, None
        for i, arg in enumerate(convert_factory.deserialize(value)):
            if i % 2 == 0:
                k = arg
            else:
                v = arg
                result[k] = v
        return result

# id 10
@convert_factory.register(np.ndarray)
class NDArrayConverter(Converter):
    def serialize(self, value: np.ndarray) -> List[bytes]:
        result = []
        shape = [int(dim) for dim in value.shape]
        dtype = value.dtype.name
        data = value.tobytes()
        result.extend(convert_factory.serialize(shape))
        result.extend(convert_factory.serialize(dtype))
        result.extend(convert_factory.serialize(data))
        return result

    def deserialize(self, value: bytes) -> np.ndarray:
        shape, dtype, data = convert_factory.deserialize(value)
        return np.frombuffer(data, dtype=dtype).reshape(shape)


def read_cpp_args(filename):
    with open(filename, 'rb') as f:
        data = f.read()
        return convert_factory.deserialize(data)

def dump_result(filename, *args):
    with open(filename, 'wb') as f:
        for data in convert_factory.serialize(*args):
            f.write(data)

def convert_cpp_args(func, vars):
    vars = list(vars)

    var_count = len(vars)
    func_var_count = func.__code__.co_argcount
    default_count = len(func.__defaults__) if func.__defaults__ else 0

    if var_count < func_var_count - default_count:
        print(f"cpp_entry args count not match, get {var_count} "
              f"need min count {func_var_count - default_count}")
        raise ValueError("cpp_entry args count not match")

    if var_count > func_var_count:
        print(f"cpp_entry args count not match, get {var_count} "
              f"over max count {func_var_count}")
        raise ValueError("cpp_entry args count not match")

    for var_name, var in zip(func.__code__.co_varnames, vars):
        if var_name not in func.__annotations__:
            yield var
        else:
            if func.__annotations__[var_name].__module__ == 'typing':
                yield var
            else:
                yield func.__annotations__[var_name](var)

def cpp_entry(func):
    def wrapper(*args, **kwargs):
        return func(*args, **kwargs)

    if len(sys.argv) != 4:
        return wrapper

    _, funcname, filename, magic = sys.argv
    if magic != "PYFUNC_CALL" or funcname != func.__name__:
        return wrapper

    try:
        func_args = read_cpp_args(filename)
        func_args = convert_cpp_args(func, func_args)
        result = func(*func_args)
        if isinstance(result, tuple):
            dump_result(filename, *result)
        else:
            dump_result(filename, result)
    except Exception as e:
        traceback.print_exc()
        print(f"run python {funcname} error={e}")
        exit(-1)
    else:
        exit(0)
