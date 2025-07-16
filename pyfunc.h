#pragma once

#include <chrono>
#include <cstdint>
#include <cstring>
#include <ios>
#include <iostream>
#include <type_traits>
#include <vector>
#include <tuple>
#include <cassert>
#include <fstream>
#include <map>
#include <chrono>
#include <sstream>
#include <thread>
#include <memory>
#include <functional>


class SerializeSize {
 public:
  template<typename T>
  size_t operator()(const T& value);

  template<typename T, typename... Args>
  size_t operator()(const T& value, const Args&... args);
};

class Serialize {
 public:
  Serialize(std::ostream& os) : os_(os) {}
  ~Serialize() { os_.flush(); }

  bool operator()();

  template<typename T>
  bool operator()(const T& value);

  template<typename T, typename... Args>
  bool operator()(const T& value, const Args&... args);

 private:
  std::ostream& os_;
};

class Deserialize {
 public:
  Deserialize(const std::string& filename)
      : ifs_(filename.c_str(), std::ios_base::in | std::ios::binary),
        is_(ifs_), tellg_(0), offset_(0) {
    if (!ifs_.good())
      throw std::runtime_error("Invalid input stream");
    ifs_.seekg(0, std::ios::end);
    size_ = ifs_.tellg();
    ifs_.seekg(0, std::ios::beg);
  }

  Deserialize(std::istream& is, size_t size)
      : is_(is), size_(size), offset_(0) {
    tellg_ = is_.tellg();
  }

  template<typename T>
  T single();

  template<typename... Args>
  std::tuple<Args...> multi();

  template<typename T>
  bool is();

  bool empty() {
    return offset_ >= size_;
  }

  void reset() { 
    offset_ = 0;
    is_.seekg(tellg_, std::ios::beg);
  }

 private:
  std::fstream ifs_;
  std::istream& is_;
  size_t tellg_{0};
  size_t offset_{0};
  size_t size_{0};
};

template<typename T>
struct ConvertID {
  const static constexpr uint32_t id = 0;
};

template<typename T>
struct Cast {
  using type = T;
  type to(const T& value) { return value; }
  T from(type&& value) { return value; }
};

template<typename T>
struct Converter {
  bool serialize(const T& value, std::ostream& os);
  size_t serializeSize(const T& value);
  T deserialize(std::istream& is);
};

template<typename T>
bool type2buff(T value, std::ostream& os) {
  os.write(reinterpret_cast<const char*>(&value), sizeof(T));
  return true;
}

template<typename T>
T buff2type(std::istream& is) {
  T value;
  is.read(reinterpret_cast<char*>(&value), sizeof(T));
  return value;
}

template <typename Tuple, typename Func, size_t ... N>
void TupleCall(Tuple& t, Func&& func, std::index_sequence<N...>) {
  static_cast<void>(std::initializer_list<int>{(func(std::get<N>(t)), 0)...});
}

template <typename ... Args, typename Func>
void TupleTravel(std::tuple<Args...>& t, Func&& func) {
  TupleCall(t, std::forward<Func>(func),
               std::make_index_sequence<sizeof...(Args)>{});
}

template <typename Tuple, typename Func, size_t ... N>
void TupleCall(const Tuple& t, Func&& func, std::index_sequence<N...>) {
  static_cast<void>(std::initializer_list<int>{(func(std::get<N>(t)), 0)...});
}

template <typename ... Args, typename Func>
void TupleTravel(const std::tuple<Args...>& t, Func&& func) {
  TupleCall(t, std::forward<Func>(func),
               std::make_index_sequence<sizeof...(Args)>{});
}

// ==== None ====
struct None { };

template<>
struct ConvertID<None> {
  const static constexpr uint32_t id = 1;
};

template<>
struct Converter<None> {
  bool serialize(None value, std::ostream& os) {
    return type2buff<uint64_t>(0, os);
  }

  size_t serializeSize(None value) {
    return 0;
  }

  None deserialize(std::istream& is, size_t size) {
    assert(size == 0 && "invalid size");
    return None();
  }
};

// ==== bool ====
template<>
struct ConvertID<bool> {
  const static constexpr uint32_t id = 2;
};

template<>
struct Converter<bool> {
  bool serialize(bool value, std::ostream& os) {
    return type2buff<bool>(value, os);
  }

  size_t serializeSize(bool value) {
    return sizeof(bool);
  }

  bool deserialize(std::istream& is, size_t size) {
    assert(size == sizeof(bool) && "invalid size");
    return buff2type<bool>(is);
  }
};

// ==== int ====
template<>
struct ConvertID<int64_t> {
  const static constexpr uint32_t id = 3;
};

template<>
struct Converter<int64_t> {
  bool serialize(int64_t value, std::ostream& os) {
    return type2buff<int64_t>(value, os);
  }

  size_t serializeSize(int64_t value) {
    return sizeof(int64_t);
  }

  int64_t deserialize(std::istream& is, size_t size) {
    assert(size == sizeof(int64_t) && "invalid size");
    return buff2type<int64_t>(is);
  }
};

template<>
struct Cast<uint64_t> {
  using type = int64_t;
  type to(uint64_t value) { return value; }
  uint64_t from(type value) { return value; }
};

template<>
struct Cast<int32_t> {
  using type = int64_t;
  type to(int32_t value) { return value; }
  int32_t from(type value) { return value; }
};

template<>
struct Cast<uint32_t> {
  using type = int64_t;
  type to(uint32_t value) { return value; }
  uint32_t from(type value) { return value; }
};

template<>
struct Cast<int16_t> {
  using type = int64_t;
  type to(int16_t value) { return value; }
  int16_t from(type value) { return value; }
};

template<>
struct Cast<int8_t> {
  using type = int64_t;
  type to(int8_t value) { return value; }
  int8_t from(type value) { return value; }
};

template<>
struct Cast<uint8_t> {
  using type = int64_t;
  type to(int8_t value) { return value; }
  uint8_t from(type value) { return value; }
};

// ==== float ====
template<>
struct ConvertID<double> {
  const static constexpr uint32_t id = 4;
};

template<>
struct Converter<double> {
  bool serialize(double value, std::ostream& os) {
    return type2buff<double>(value, os);
  }

  size_t serializeSize(double value) {
    return sizeof(double);
  }

  double deserialize(std::istream& is, size_t size) {
    assert(size == sizeof(double) && "invalid size");
    return buff2type<double>(is);
  }
};

template<>
struct Cast<float> {
  using type = double;
  type to(float value) { return value; }
  float from(type value) { return value; }
};

// ==== str ====
template<>
struct ConvertID<std::string> {
  const static constexpr uint32_t id = 5;
};

template<>
struct Converter<std::string> {
  bool serialize(const std::string& value, std::ostream& os) {
    os.write(value.data(), value.size());
    return true;
  }

  size_t serializeSize(const std::string& value) {
    return value.size();
  }

  std::string deserialize(std::istream& is, size_t size) {
    std::string value;
    value.resize(size);
    is.read(const_cast<char*>(value.data()), size);
    return value;
  }
};

template<>
struct Cast<char> {
  using type = std::string;
  type to(char value) {
    std::string result;
    result += value;
    return result;
  }
  char from(std::string&& value) {
    assert(value.size() == 1);
    return value[0];
  }
};

template<>
struct Cast<char *> {
  using type = std::string;
  type to(const char* value) {
    return value;
  }
  char* from(std::string&& value) {
    assert(false && "not support convert to char*");
    return nullptr;
  }
};

template<>
struct Cast<const char *> {
  using type = std::string;
  type to(const char* value) {
    return value;
  }
  char* from(std::string&& value) {
    assert(false && "not support convert to const char*");
    return nullptr;
  }
};

template<int N>
struct Cast<char[N]> {
  using type = std::string;
  type to(const char (&value)[N]) {
    return value;
  }
  char* from(std::string&& value) {
    assert(false && "not support convert to char[]");
    return nullptr;
  }
};

// ==== bytes ====
template<>
struct ConvertID<std::vector<char>> {
  const static constexpr uint32_t id = 6;
};

template<>
struct Converter<std::vector<char>> {
  bool serialize(const std::vector<char>& value, std::ostream& os) {
    os.write(value.data(), value.size());
    return true;
  }

  size_t serializeSize(const std::vector<char>& value) {
    return value.size();
  }

  std::vector<char> deserialize(std::istream& is, size_t size) {
    std::vector<char> value(size);
    is.read(value.data(), size);
    return value;
  }
};

// ==== list ====
template<typename T>
struct ConvertID<std::vector<T>> {
  const static constexpr uint32_t id = 7;
};

template<typename T>
struct Converter<std::vector<T>> {
  bool serialize(const std::vector<T>& value, std::ostream& os) {
    Serialize ser(os);
    for (auto& v: value) {
      if (!ser(v)) return false;
    }
    return true;
  }

  size_t serializeSize(const std::vector<T>& value) {
    size_t size = 0;
    SerializeSize sersize;
    for (auto& v: value)
      size += sersize(v);
    return size;
  }

  std::vector<T> deserialize(std::istream& is, size_t size) {
    Deserialize deser(is, size);
    std::vector<T> result;
    while (!deser.empty()) {
      assert(deser.is<T>());
      result.emplace_back(deser.single<T>());
    }
    return result;
  }
};

// ==== tuple ====
template<typename... Args>
struct ConvertID<std::tuple<Args...>> {
  const static constexpr uint32_t id = 8;
};

template<typename... Args>
struct Converter<std::tuple<Args...>> {
  bool serialize(const std::tuple<Args...>& value, std::ostream& os) {
    bool success = true;

    Serialize ser(os);
    TupleTravel(value, [&ser, &success](auto& value) {
      success &= ser.operator()<
        std::remove_cv_t<std::remove_reference_t<decltype(value)>>
      >(value);
    });
    return success;
  }

  size_t serializeSize(const std::tuple<Args...>& value) {
    size_t size = 0;
    SerializeSize sersize;
    TupleTravel(value, [&sersize, &size](auto& value) {
      size +=  sersize(value);
    });
    return size;
  }

  std::tuple<Args...> deserialize(std::istream& is, size_t size) {
    Deserialize deser(is, size);
    return deser.multi<Args...>();
  }
};

// ==== dict ====
template<typename K, typename V>
struct ConvertID<std::map<K, V>> {
  const static constexpr uint32_t id = 9;
};

template<typename K, typename V>
struct Converter<std::map<K, V>> {
  bool serialize(const std::map<K, V>& value, std::ostream& os) {
    Serialize ser(os);
    for (auto& pair: value) {
      if (!ser(pair.first))
        return false;
      if (!ser(pair.second))
        return false;
    }
    return true;
  }

  size_t serializeSize(const std::map<K, V>& value) {
    size_t size = 0;
    SerializeSize sersize;
    for (auto& pair: value) {
      size += sersize(pair.first);
      size += sersize(pair.second);
    }
    return size;
  }

  std::map<K, V> deserialize(std::istream& is, size_t size) {
    std::map<K, V> result;
    Deserialize deser(is, size);
    while (!deser.empty()) {
      auto key = deser.single<K>();
      auto value = deser.single<V>();
      result.emplace(std::move(key), std::move(value));
    }
    return result;
  }
};

// ==== ndarray ====
struct NDArray {
  std::vector<int64_t> shape;
  std::string dtype;
  std::vector<char> data;
};

template<>
struct ConvertID<NDArray> {
  const static constexpr uint32_t id = 10;
};

template<>
struct Converter<NDArray> {
  bool serialize(const NDArray& value, std::ostream& os) {
    Serialize ser(os);
    if (!ser(value.shape))
      return false;
    if (!ser(value.dtype))
      return false;
    if (!ser(value.data))
      return false;
    return true;
  }

  size_t serializeSize(const NDArray& value) {
    size_t size = 0;
    SerializeSize sersize;
    size += sersize(value.shape);
    size += sersize(value.dtype);
    size += sersize(value.data);
    return size;
  }

  NDArray deserialize(std::istream& is, size_t size) {
    Deserialize deser(is, size);
    return {
      deser.single<std::vector<int64_t>>(),
      deser.single<std::string>(),
      deser.single<std::vector<char>>()
    };
  }
};


bool Serialize::operator()() {
  return true;
}

template<typename T>
bool Serialize::operator()(const T& value) {
  using CT = typename Cast<T>::type;

  if (!type2buff<uint32_t>(ConvertID<CT>::id, os_))
    return false;
  
  if (std::is_same<CT, T>::value) {
    size_t size = Converter<CT>().serializeSize(value);
    if (!type2buff<size_t>(size, os_))
      return false;
    return Converter<CT>().serialize(value, os_);
  } 

  auto cast = Cast<T>();
  auto cast_value = cast.to(value);
  size_t size = Converter<CT>().serializeSize(cast_value);
  if (!type2buff<size_t>(size, os_))
    return false;
  return Converter<CT>().serialize(cast_value, os_);
}

template<typename T, typename... Args>
bool Serialize::operator()(const T& value, const Args&... args) {
    if (!operator()<T>(value))
      return false;
    return operator()<Args...>(args...);
}

template<typename T>
size_t SerializeSize::operator()(const T& value) {
    using CT = typename Cast<T>::type;

    size_t size = sizeof(uint32_t) + sizeof(size_t);

    if (std::is_same<CT, T>::value) {
      size += Converter<CT>().serializeSize(value);
    } else {
      size += Converter<CT>().serializeSize(Cast<T>().to(value));
    }
    return size;
}

template<typename T, typename... Args>
size_t SerializeSize::operator()(const T& value, const Args&... args) {
  size_t size = operator()<T>(value);
  size += operator()<Args...>(args...);
  return size;
}

template<typename T>
T Deserialize::single() {
  assert(!empty() && "empty buffer");
  using CT = typename Cast<T>::type;

  uint32_t id = buff2type<uint32_t>(is_);

  if (id != ConvertID<CT>::id) {
    std::cout << "deserialize failed, id=" << id 
              << " expect=" << ConvertID<CT>::id
              << std::endl;
    assert(false && "id not match");
  }
  offset_ += sizeof(uint32_t);

  size_t size = buff2type<size_t>(is_);
  offset_ += sizeof(size_t);

  auto value = Converter<CT>().deserialize(is_, size);
  offset_ += size;

  if (std::is_same<CT, T>::value)
    return value;
  return Cast<T>().from(std::move(value));
}

template<typename... Args>
std::tuple<Args...> Deserialize::multi() {
  std::tuple<Args...> result;
  TupleTravel(result, [this](auto& value) {
    value = single<std::remove_reference_t<decltype(value)>>();
  });
  return result;
}

template<typename T>
bool Deserialize::is() {
  if (empty()) return false;
  using CT = typename Cast<T>::type;
  uint32_t id = buff2type<uint32_t>(is_);
  for (size_t i = 0; i < sizeof(id); i++)
    is_.unget();
  return id == ConvertID<CT>::id;
}


class PyFunc {
public:
  PyFunc(std::string modulename, std::string funcname,
      std::string python="python3")
    : modulename_(std::move(modulename)), funcname_(std::move(funcname)) {
    initTmpFileName();
    cmd_ = python + " -m "  +
           modulename_ + " " +
           funcname_ + " " +
           tmpfile_ + " " +
           "PYFUNC_CALL"; // for magic check
    enable_timer_ = std::getenv("PYFUNC_ENABLE_TIMER");
  }

  ~PyFunc() {
    std::remove(tmpfile_.c_str());
  }

  template<typename... Args>
  bool call(const Args&... args) {
    if (!serializeArgs(args...)) {
      std::cout << "[PyFunc][" << modulename_ << "::" << funcname_
                << "] serialize args failed" << std::endl;
      return false;
    }

    int status = 0;
    timer("python run", [this, &status]() {
      status = std::system(cmd_.c_str());
    });

    if (status != 0) {
      std::cout << "[PyFunc][" << modulename_ << "::" << funcname_
                  << "] run cmd failed, error=" << status << std::endl;
      std::remove(tmpfile_.c_str());
      return false;
    }
    deser_ = std::make_unique<Deserialize>(tmpfile_);
    return true;
  }

  template<typename... Args>
  std::tuple<Args...> gets() {
    assert(deser_ && "call first or python run failed");
    return deser_->multi<Args...>();
  }

  template<typename T>
  T get() { 
    assert(deser_ && "call first or python run failed");
    T result;
    timer("deserialize", [this, &result]() {
        result = deser_->single<T>();
    });
    return result; 
  }

  template<typename T>
  bool is() {
    assert(deser_ && "call first or python run failed");
    return deser_->is<T>();
  }

  void reset() {
    assert(deser_ && "call first or python run failed");
    deser_->reset();
  }

private:
  void timer(const char* msg, std::function<void()> func) {
    if (!enable_timer_) {
      func();
      return;
    }
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "[PyFunc][" << modulename_ << "::" << funcname_
              << "] " << msg << " elapsed time: "
              << duration.count() << "ms" << std::endl;
  }

  template<typename... Args>
  bool serializeArgs(const Args&... args) {
    std::remove(tmpfile_.c_str());
    std::ofstream out(tmpfile_.c_str(),
                      std::ios_base::out | std::ios_base::binary);
    if (!out.is_open()) {
      std::cout << "[PyFunc][" << modulename_ << "::" << funcname_
                << "] write tmp file failed!\n";
      return false;
    }
    bool status = true;
    timer("serialize", [this, &out, &args..., &status]() {
      Serialize serialize(out);
      status = serialize(args...);
    });
    out.close();
    return status;
  }

  void initTmpFileName() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    long long timestamp =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    std::stringstream ss;
    ss << ".tmp" << modulename_
       << funcname_ << timestamp
       << std::this_thread::get_id();
    tmpfile_ = ss.str();
  }

 private:
  std::string modulename_;
  std::string funcname_;
  std::string tmpfile_;
  std::unique_ptr<Deserialize> deser_{nullptr};
  std::string cmd_;
  bool enable_timer_{false};
};
