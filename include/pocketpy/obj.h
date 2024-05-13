#pragma once

#include "common.h"
#include "namedict.h"
#include "tuplelist.h"

namespace pkpy {
    
struct Frame;
class VM;

#if PK_ENABLE_STD_FUNCTION
using NativeFuncC = std::function<PyVar(VM*, ArgsView)>;
#else
typedef PyVar (*NativeFuncC)(VM*, ArgsView);
#endif

enum class BindType{
    DEFAULT,
    STATICMETHOD,
    CLASSMETHOD,
};

struct BoundMethod {
    PyVar self;
    PyVar func;
    BoundMethod(PyVar self, PyVar func) : self(self), func(func) {}
};

struct StaticMethod{
    PyVar func;
    StaticMethod(PyVar func) : func(func) {}
};

struct ClassMethod{
    PyVar func;
    ClassMethod(PyVar func) : func(func) {}
};

struct Property{
    PyVar getter;
    PyVar setter;
    Property(PyVar getter, PyVar setter) : getter(getter), setter(setter) {}
};

struct Range {
    i64 start = 0;
    i64 stop = -1;
    i64 step = 1;
};

struct StarWrapper{
    int level;      // either 1 or 2
    PyVar obj;
    StarWrapper(int level, PyVar obj) : level(level), obj(obj) {}
};

struct Bytes{
    unsigned char* _data;
    int _size;

    int size() const noexcept { return _size; }
    int operator[](int i) const noexcept { return (int)_data[i]; }
    const unsigned char* data() const noexcept { return _data; }

    bool operator==(const Bytes& rhs) const;
    bool operator!=(const Bytes& rhs) const;

    Str str() const noexcept { return Str((char*)_data, _size); }
    std::string_view sv() const noexcept { return std::string_view((char*)_data, _size); }

    Bytes() : _data(nullptr), _size(0) {}
    Bytes(unsigned char* p, int size): _data(p), _size(size) {}
    Bytes(const Str& str): Bytes(str.sv()) {}
    operator bool() const noexcept { return _data != nullptr; }

    Bytes(std::string_view sv);
    Bytes(const Bytes& rhs);
    Bytes(Bytes&& rhs) noexcept;
    Bytes& operator=(Bytes&& rhs) noexcept;
    Bytes& operator=(const Bytes& rhs) = delete;
    std::pair<unsigned char*, int> detach() noexcept;

    ~Bytes(){ delete[] _data;}
};

using Super = std::pair<PyVar, Type>;

struct Slice {
    PyVar start;
    PyVar stop;
    PyVar step;
    Slice(PyVar start, PyVar stop, PyVar step) : start(start), stop(stop), step(step) {}
};

struct PyObject{
    bool gc_enabled;    // whether this object is managed by GC
    bool gc_marked;     // whether this object is marked
    NameDict* _attr;

    bool is_attr_valid() const noexcept { return _attr != nullptr; }

    NameDict& attr() {
        PK_DEBUG_ASSERT(is_attr_valid())
        return *_attr;
    }

    PyVar attr(StrName name) const {
        PK_DEBUG_ASSERT(is_attr_valid())
        return (*_attr)[name];
    }

    virtual void _obj_gc_mark() = 0;
    virtual ~PyObject();

    PyObject() : gc_enabled(true), gc_marked(false), _attr(nullptr) {}

    void _enable_instance_dict() {
        _attr = new(pool128_alloc<NameDict>()) NameDict();
    }

    void _enable_instance_dict(float lf){
        _attr = new(pool128_alloc<NameDict>()) NameDict(lf);
    }
};

const int kTpIntIndex = 3;
const int kTpFloatIndex = 4;

inline bool is_tagged(PyVar p) noexcept { return p.is_sso; }
inline bool is_float(PyVar p) noexcept { return p.type.index == kTpFloatIndex; }
inline bool is_int(PyVar p) noexcept { return p.type.index == kTpIntIndex; }

inline bool is_type(PyVar obj, Type type) {
    PK_DEBUG_ASSERT(obj != nullptr)
    return obj.type == type;
}

template <typename, typename=void> struct has_gc_marker : std::false_type {};
template <typename T> struct has_gc_marker<T, std::void_t<decltype(&T::_gc_mark)>> : std::true_type {};

template <typename T>
struct Py_ final: PyObject {
    static_assert(!std::is_reference_v<T>);
    static_assert(!is_sso_v<T>);

    T _value;
    void _obj_gc_mark() override {
        if constexpr (has_gc_marker<T>::value) {
            _value._gc_mark();
        }
    }

    template <typename... Args>
    Py_(Args&&... args) : PyObject(), _value(std::forward<Args>(args)...) { }
};

struct MappingProxy{
    PyVar obj;
    MappingProxy(PyVar obj) : obj(obj) {}
    NameDict& attr() { return obj->attr(); }
};

void _gc_mark_namedict(NameDict*);
StrName _type_name(VM* vm, Type type);
template<typename T> T to_void_p(VM*, PyVar);
PyVar from_void_p(VM*, void*);


template<typename T>
obj_get_t<T> PyVar::obj_get(){
    if constexpr(is_sso_v<T>){
        return as<T>();
    }else{
        return ((Py_<T>*)(get()))->_value;
    }
}

#define PK_OBJ_GET(T, obj) (obj).obj_get<T>()

#define PK_OBJ_MARK(obj) \
    if(!is_tagged(obj) && !(obj)->gc_marked) {                          \
        (obj)->gc_marked = true;                                        \
        (obj)->_obj_gc_mark();                                          \
        if((obj)->is_attr_valid()) _gc_mark_namedict((obj)->_attr);     \
    }

#define VAR(x) py_var(vm, x)
#define CAST(T, x) py_cast<T>(vm, x)
#define _CAST(T, x) _py_cast<T>(vm, x)

#define CAST_F(x) py_cast<f64>(vm, x)
#define CAST_DEFAULT(T, x, default_value) (x != vm->None) ? py_cast<T>(vm, x) : (default_value)

/*****************************************************************/
inline bool try_cast_int(PyVar obj, i64* val) noexcept {
    if(is_int(obj)){
        *val = obj.as<i64>();
        return true;
    }
    return false;
}

template<>
struct Py_<List> final: PyObject {
    List _value;
    Py_(List&& val): PyObject(), _value(std::move(val)) {}
    Py_(const List& val): PyObject(), _value(val) {}

    void _obj_gc_mark() override {
        for(PyVar obj: _value) PK_OBJ_MARK(obj);
    }
};

template<>
struct Py_<Tuple> final: PyObject {
    Tuple _value;
    Py_(Tuple&& val): PyObject(), _value(std::move(val)) {}
    Py_(const Tuple& val): PyObject(), _value(val) {}

    void _obj_gc_mark() override {
        for(PyVar obj: _value) PK_OBJ_MARK(obj);
    }
};

template<>
struct Py_<MappingProxy> final: PyObject {
    MappingProxy _value;
    Py_(MappingProxy val): PyObject(), _value(val) {}
    void _obj_gc_mark() override {
        PK_OBJ_MARK(_value.obj);
    }
};

template<>
struct Py_<BoundMethod> final: PyObject {
    BoundMethod _value;
    Py_(BoundMethod val): PyObject(), _value(val) {}
    void _obj_gc_mark() override {
        PK_OBJ_MARK(_value.self);
        PK_OBJ_MARK(_value.func);
    }
};

template<>
struct Py_<StarWrapper> final: PyObject {
    StarWrapper _value;
    Py_(StarWrapper val): PyObject(), _value(val) {}
    void _obj_gc_mark() override {
        PK_OBJ_MARK(_value.obj);
    }
};

template<>
struct Py_<StaticMethod> final: PyObject {
    StaticMethod _value;
    Py_(StaticMethod val): PyObject(), _value(val) {}
    void _obj_gc_mark() override {
        PK_OBJ_MARK(_value.func);
    }
};

template<>
struct Py_<ClassMethod> final: PyObject {
    ClassMethod _value;
    Py_(ClassMethod val): PyObject(), _value(val) {}
    void _obj_gc_mark() override {
        PK_OBJ_MARK(_value.func);
    }
};

template<>
struct Py_<Property> final: PyObject {
    Property _value;
    Py_(Property val): PyObject(), _value(val) {}
    void _obj_gc_mark() override {
        PK_OBJ_MARK(_value.getter);
        PK_OBJ_MARK(_value.setter);
    }
};

template<>
struct Py_<Slice> final: PyObject {
    Slice _value;
    Py_(Slice val): PyObject(), _value(val) {}
    void _obj_gc_mark() override {
        PK_OBJ_MARK(_value.start);
        PK_OBJ_MARK(_value.stop);
        PK_OBJ_MARK(_value.step);
    }
};

template<>
struct Py_<Super> final: PyObject {
    Super _value;
    template<typename... Args>
    Py_(Args&&... args): PyObject(), _value(std::forward<Args>(args)...) {}
    void _obj_gc_mark() override {
        PK_OBJ_MARK(_value.first);
    }
};

template<>
struct Py_<DummyInstance> final: PyObject {
    Py_(): PyObject() { _enable_instance_dict(); }
    void _obj_gc_mark() override {}
};

template<>
struct Py_<Type> final: PyObject {
    Type _value;
    Py_(Type val): PyObject(), _value(val) {
        _enable_instance_dict(PK_TYPE_ATTR_LOAD_FACTOR);
    }
    void _obj_gc_mark() override {}
};

template<>
struct Py_<DummyModule> final: PyObject {
    Py_(): PyObject() {
        _enable_instance_dict(PK_TYPE_ATTR_LOAD_FACTOR);
    }
    void _obj_gc_mark() override {}
};

extern PyVar const PY_NULL;
extern PyVar const PY_OP_CALL;
extern PyVar const PY_OP_YIELD;

}   // namespace pkpy