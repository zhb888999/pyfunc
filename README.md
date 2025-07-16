# pyfunc
在c++中调用python函数工具

## 功能
- 调用python函数
- 序列化python函数的参数
- 反序列化python函数的返回值

## 依赖
- python3.6+
- numpy

## 使用方法
python端之前需要import cpp_entry中的cpp_entry装饰器，然后在python端定义函数，
并使用cpp_entry装饰器进行装饰即可

cpp_entry_with_cache装饰器可以缓存函数的返回值，减少重复计算，
具体使用方法请参考main.py中的代码

c++端需要包含pyfunc.h头文件，使用PyFunc类进行调用即可，构造函数需要传入模块名和函数名，
调用call方法传入参数， get方法获取返回值，gets方法获取多个返回值，is方法判断返回值类型是否正确，
reset方法重置返回值， 具体使用方法请参考main.cpp中的代码

## 支持自定义类型
分别在c++和python定义转化类型，类型id保持一致，并实现序列化和反序列化接口即可,
具体请参考其它类型实现方法