#ifndef SRC_LLV8_H_
#define SRC_LLV8_H_

#include <cstring>
#include <string>

#include <lldb/API/LLDB.h>

#include "src/error.h"
#include "src/llv8-constants.h"

namespace llnode {

namespace node {
namespace constants {
class Environment;
}
}  // namespace node

class FindJSObjectsVisitor;
class FindReferencesCmd;
class FindObjectsCmd;

namespace v8 {

// Forward declarations
class LLV8;
class CodeMap;


#define V8_VALUE_DEFAULT_METHODS(NAME, PARENT)     \
  NAME(const NAME& v) = default;                   \
  NAME() : PARENT() {}                             \
  NAME(LLV8* v8, int64_t raw) : PARENT(v8, raw) {} \
  NAME(Value& v) : PARENT(v) {}                    \
  NAME(Value* v) : PARENT(v->v8(), v->raw()) {}    \
  static inline const char* ClassName() { return #NAME; }

class Value {
 public:
  class InspectOptions {
   public:
    InspectOptions()
        : detailed(false),
          print_map(false),
          print_source(false),
          length(kLength),
          indent_depth(1),
          output_limit(0) {}

    static const unsigned int kLength = 16;
    static const unsigned int kIndentSize = 2;
    inline std::string get_indent_spaces() {
      return std::string(indent_depth * kIndentSize, ' ');
    }

    bool detailed;
    bool print_map;
    bool print_source;
    unsigned int length;
    unsigned int indent_depth;
    int output_limit;
  };

  Value(const Value& v) = default;
  Value(Value& v) = default;
  Value() : v8_(nullptr), raw_(-1) {}
  Value(LLV8* v8, int64_t raw) : v8_(v8), raw_(raw) {}

  inline bool Check() const { return true; }

  inline int64_t raw() const { return raw_; }
  inline LLV8* v8() const { return v8_; }

  bool IsHoleOrUndefined(Error& err);
  bool IsHole(Error& err);

  std::string Inspect(InspectOptions* options, Error& err);
  std::string GetTypeName(Error& err);
  std::string ToString(Error& err);

  static inline const char* ClassName() { return "Value"; }

 protected:
  LLV8* v8_;
  int64_t raw_;
};

class Smi : public Value {
 public:
  V8_VALUE_DEFAULT_METHODS(Smi, Value)

  inline bool Check() const;
  inline int64_t GetValue() const;

  std::string ToString(Error& err);
  std::string Inspect(Error& err);
};

class HeapObject : public Value {
 public:
  V8_VALUE_DEFAULT_METHODS(HeapObject, Value)

  inline bool Check() const;
  inline int64_t LeaField(int64_t off) const;
  inline int64_t LoadField(int64_t off, Error& err);

  template <class T>
  inline T LoadFieldValue(int64_t off, Error& err);

  inline HeapObject GetMap(Error& err);
  inline int64_t GetType(Error& err);

  std::string ToString(Error& err);
  std::string Inspect(InspectOptions* options, Error& err);
  std::string GetTypeName(Error& err);

  inline bool IsJSErrorType(Error& err);
};

class Map : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(Map, HeapObject)

  inline int64_t GetType(Error& err);
  inline HeapObject MaybeConstructor(Error& err);
  inline HeapObject InstanceDescriptors(Error& err);
  inline int64_t BitField3(Error& err);
  inline int64_t InObjectProperties(Error& err);
  inline int64_t ConstructorFunctionIndex(Error& err);
  inline int64_t InstanceSize(Error& err);
  inline int64_t InstanceType(Error& err);

  inline bool IsDictionary(Error& err);
  inline bool IsJSObjectMap(Error& err);
  inline int64_t NumberOfOwnDescriptors(Error& err);

  std::string Inspect(InspectOptions* options, Error& err);
  HeapObject Constructor(Error& err);
};

class String : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(String, HeapObject)

  inline int64_t Encoding(Error& err);
  inline int64_t Representation(Error& err);
  inline Smi Length(Error& err);

  std::string ToString(Error& err);
  std::string Inspect(InspectOptions* options, Error& err);

  static inline bool IsString(LLV8* v8, HeapObject heap_object, Error& err);
};

class Script : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(Script, HeapObject)

  inline String Name(Error& err);
  inline Smi LineOffset(Error& err);
  inline HeapObject Source(Error& err);
  inline HeapObject LineEnds(Error& err);

  void GetLines(uint64_t start_line, std::string lines[], uint64_t line_limit,
                uint32_t& lines_found, Error& err);
  void GetLineColumnFromPos(int64_t pos, int64_t& line, int64_t& column,
                            Error& err);
};

class Code : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(Code, HeapObject)

  inline int64_t Start();
  inline int64_t Size(Error& err);
};

class SharedFunctionInfo : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(SharedFunctionInfo, HeapObject)

  inline String Name(Error& err);
  inline Value InferredName(Error& err);
  inline Script GetScript(Error& err);
  inline HeapObject GetScopeInfo(Error& err);
  inline int64_t ParameterCount(Error& err);
  inline int64_t StartPosition(Error& err);
  inline int64_t EndPosition(Error& err);

  std::string ProperName(Error& err);
  std::string GetPostfix(Error& err);
  std::string ToString(Error& err);

 private:
  inline String name(Error& err);
  inline HeapObject scope_info(Error& err);
  inline HeapObject name_or_scope_info(Error& err);
};

class OneByteString : public String {
 public:
  V8_VALUE_DEFAULT_METHODS(OneByteString, String)

  inline std::string ToString(Error& err);
};

class TwoByteString : public String {
 public:
  V8_VALUE_DEFAULT_METHODS(TwoByteString, String)

  inline std::string ToString(Error& err);
};

class ConsString : public String {
 public:
  V8_VALUE_DEFAULT_METHODS(ConsString, String)

  inline String First(Error& err);
  inline String Second(Error& err);

  inline std::string ToString(Error& err);
};

class SlicedString : public String {
 public:
  V8_VALUE_DEFAULT_METHODS(SlicedString, String)

  inline String Parent(Error& err);
  inline Smi Offset(Error& err);

  inline std::string ToString(Error& err);
};

class ThinString : public String {
 public:
  V8_VALUE_DEFAULT_METHODS(ThinString, String)

  inline String Actual(Error& err);

  inline std::string ToString(Error& err);
};

class HeapNumber : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(HeapNumber, HeapObject)

  inline double GetValue(Error& err);

  std::string ToString(bool whole, Error& err);
  std::string Inspect(Error& err);
};

class JSObject : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(JSObject, HeapObject);

  inline HeapObject Properties(Error& err);
  inline HeapObject Elements(Error& err);

  std::string Inspect(InspectOptions* options, Error& err);
  virtual std::string InspectAllProperties(InspectOptions* options, Error& err);
  std::string InspectInternalFields(Error& err);
  std::string InspectProperties(Error& err);

  std::string InspectElements(Error& err);
  std::string InspectElements(int64_t length, Error& err);
  std::string InspectDictionary(Error& err);
  std::string InspectDescriptors(Map map, Error& err);
  void Keys(std::vector<std::string>& keys, Error& err);

  /** Return all the key/value pairs for properties on a JSObject
   * This allows keys to be inflated to JSStrings later once we know if
   * they are needed.
   */
  std::vector<std::pair<Value, Value>> Entries(Error& err);

  Value GetProperty(std::string key_name, Error& err);
  int64_t GetArrayLength(Error& err);
  Value GetArrayElement(int64_t pos, Error& err);

  static inline bool IsObjectType(LLV8* v8, int64_t type);

 protected:
  template <class T>
  T GetInObjectValue(int64_t size, int index, Error& err);
  void ElementKeys(std::vector<std::string>& keys, Error& err);
  void DictionaryKeys(std::vector<std::string>& keys, Error& err);
  void DescriptorKeys(std::vector<std::string>& keys, Map map, Error& err);
  std::vector<std::pair<Value, Value>> DictionaryEntries(Error& err);
  std::vector<std::pair<Value, Value>> DescriptorEntries(Map map, Error& err);
  Value GetDictionaryProperty(std::string key_name, Error& err);
  Value GetDescriptorProperty(std::string key_name, Map map, Error& err);
};

class JSError : public JSObject {
 public:
  V8_VALUE_DEFAULT_METHODS(JSError, JSObject);

  std::string InspectAllProperties(InspectOptions* options,
                                   Error& err) override;
};

class JSArray : public JSObject {
 public:
  V8_VALUE_DEFAULT_METHODS(JSArray, JSObject);

  inline Smi Length(Error& err);

  std::string Inspect(InspectOptions* options, Error& err);
};

class JSFunction : public JSObject {
 public:
  V8_VALUE_DEFAULT_METHODS(JSFunction, JSObject)

  inline SharedFunctionInfo Info(Error& err);
  inline HeapObject GetContext(Error& err);
  inline std::string Name(Error& err);

  std::string GetDebugLine(std::string args, Error& err);
  std::string Inspect(InspectOptions* options, Error& err);
  std::string GetSource(Error& err);
};

class JSRegExp : public JSObject {
 public:
  V8_VALUE_DEFAULT_METHODS(JSRegExp, JSObject);

  inline String GetSource(Error& err);

  std::string Inspect(InspectOptions* options, Error& err);
};

class JSDate : public JSObject {
 public:
  V8_VALUE_DEFAULT_METHODS(JSDate, JSObject);

  inline Value GetValue(Error& err);

  std::string Inspect(Error& err);
};

class FixedArrayBase : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(FixedArrayBase, HeapObject);

  inline Smi Length(Error& err);
};

class FixedArray : public FixedArrayBase {
 public:
  V8_VALUE_DEFAULT_METHODS(FixedArray, FixedArrayBase)

  template <class T>
  inline T Get(int index, Error& err);

  inline int64_t LeaData() const;

  std::string Inspect(InspectOptions* options, Error& err);

 private:
  std::string InspectContents(int length, Error& err);
};

class FixedTypedArrayBase : public FixedArrayBase {
 public:
  V8_VALUE_DEFAULT_METHODS(FixedTypedArrayBase, FixedArrayBase)

  inline int64_t GetBase(Error& err);
  inline int64_t GetExternal(Error& err);
};

class DescriptorArray : public FixedArray {
 public:
  V8_VALUE_DEFAULT_METHODS(DescriptorArray, FixedArray)

  inline Smi GetDetails(int index, Error& err);
  inline Value GetKey(int index, Error& err);

  // NOTE: Only for DATA_CONSTANT
  inline Value GetValue(int index, Error& err);

  inline bool IsFieldDetails(Smi details);
  inline bool IsDescriptorDetails(Smi details);
  inline bool IsConstFieldDetails(Smi details);
  inline bool IsDoubleField(Smi details);
  inline int64_t FieldIndex(Smi details);
};

class NameDictionary : public FixedArray {
 public:
  V8_VALUE_DEFAULT_METHODS(NameDictionary, FixedArray)

  inline Value GetKey(int index, Error& err);
  inline Value GetValue(int index, Error& err);
  inline int64_t Length(Error& err);
};

class ScopeInfo : public FixedArray {
 public:
  V8_VALUE_DEFAULT_METHODS(ScopeInfo, FixedArray)

  inline Smi ParameterCount(Error& err);
  inline Smi StackLocalCount(Error& err);
  inline Smi ContextLocalCount(Error& err);

  inline String ContextLocalName(int index, int param_count, int stack_count,
                                 Error& err);
  inline HeapObject MaybeFunctionName(Error& err);
};

class Context : public FixedArray {
 public:
  V8_VALUE_DEFAULT_METHODS(Context, FixedArray)

  inline HeapObject GetScopeInfo(Error& err);
  inline Value Previous(Error& err);
  inline Value Native(Error& err);
  inline bool IsNative(Error& err);
  template <class T>
  inline T GetEmbedderData(int64_t index, Error& err);
  inline Value ContextSlot(int index, Error& err);

  std::string Inspect(InspectOptions* options, Error& err);
  static inline bool IsContext(LLV8* v8, HeapObject heap_object, Error& err);

  // Iterator class to walk all local references on a context
  class Locals {
   public:
    class Iterator {
     public:
      Value operator*();
      const Context::Locals::Iterator operator++(int);
      bool operator!=(Context::Locals::Iterator that);

      inline Iterator(int current, Locals* outer)
          : current_(current), outer_(outer){};

      String LocalName(Error& err) {
        return outer_->scope_info_.ContextLocalName(
            current_, outer_->param_count_, outer_->stack_count_, err);
      }

      Value GetValue(Error& err) {
        return outer_->context_->ContextSlot(current_, err);
      }

     private:
      int current_;
      Locals* outer_;
    };

    Locals(Context* context, Error& err);

    Iterator begin();
    Iterator end();

   private:
    int local_count_;
    int param_count_;
    int stack_count_;
    Context* context_;
    ScopeInfo scope_info_;
  };

 private:
  inline JSFunction Closure(Error& err);
};


class Oddball : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(Oddball, HeapObject)

  inline Smi Kind(Error& err);
  inline bool IsHoleOrUndefined(Error& err);
  inline bool IsHole(Error& err);

  std::string Inspect(Error& err);
};

class JSArrayBuffer : public JSObject {
 public:
  V8_VALUE_DEFAULT_METHODS(JSArrayBuffer, JSObject)

  inline int64_t BackingStore(Error& err);
  inline int64_t BitField(Error& err);
  inline Smi ByteLength(Error& err);

  inline bool WasNeutered(Error& err);

  std::string Inspect(InspectOptions* options, Error& err);
};

class JSArrayBufferView : public JSObject {
 public:
  V8_VALUE_DEFAULT_METHODS(JSArrayBufferView, JSObject)

  inline JSArrayBuffer Buffer(Error& err);
  inline Smi ByteOffset(Error& err);
  inline Smi ByteLength(Error& err);

  std::string Inspect(InspectOptions* options, Error& err);
};

class JSFrame : public Value {
 public:
  V8_VALUE_DEFAULT_METHODS(JSFrame, Value)

  inline int64_t LeaParamSlot(int slot, int count) const;
  inline JSFunction GetFunction(Error& err);
  inline Value GetReceiver(int count, Error& err);
  inline Value GetParam(int slot, int count, Error& err);

  uint32_t GetSourceForDisplay(bool set_line, uint32_t line_start,
                               uint32_t line_limit, std::string lines[],
                               uint32_t& lines_found, Error& err);
  std::string Inspect(bool with_args, Error& err);
  std::string InspectArgs(JSFunction fn, Error& err);

 private:
  Smi FromFrameMarker(Value value) const;
};

class LLV8 {
 public:
  LLV8() : target_(lldb::SBTarget()) {}

  void Load(lldb::SBTarget target);

 private:
  template <class T>
  inline T LoadValue(int64_t addr, Error& err);

  int64_t LoadConstant(const char* name);
  int64_t LoadPtr(int64_t addr, Error& err);
  int64_t LoadUnsigned(int64_t addr, uint32_t byte_size, Error& err);
  double LoadDouble(int64_t addr, Error& err);
  std::string LoadBytes(int64_t addr, int64_t length, Error& err);
  std::string LoadString(int64_t addr, int64_t length, Error& err);
  std::string LoadTwoByteString(int64_t addr, int64_t length, Error& err);
  uint8_t* LoadChunk(int64_t addr, int64_t length, Error& err);

  lldb::SBTarget target_;
  lldb::SBProcess process_;

  constants::Common common;
  constants::Smi smi;
  constants::HeapObject heap_obj;
  constants::Map map;
  constants::JSObject js_object;
  constants::HeapNumber heap_number;
  constants::JSArray js_array;
  constants::JSFunction js_function;
  constants::SharedInfo shared_info;
  constants::Code code;
  constants::ScopeInfo scope_info;
  constants::Context context;
  constants::Script script;
  constants::String string;
  constants::OneByteString one_byte_string;
  constants::TwoByteString two_byte_string;
  constants::ConsString cons_string;
  constants::SlicedString sliced_string;
  constants::ThinString thin_string;
  constants::FixedArrayBase fixed_array_base;
  constants::FixedTypedArrayBase fixed_typed_array_base;
  constants::FixedArray fixed_array;
  constants::Oddball oddball;
  constants::JSArrayBuffer js_array_buffer;
  constants::JSArrayBufferView js_array_buffer_view;
  constants::JSRegExp js_regexp;
  constants::JSDate js_date;
  constants::DescriptorArray descriptor_array;
  constants::NameDictionary name_dictionary;
  constants::Frame frame;
  constants::Types types;

  friend class Value;
  friend class JSFrame;
  friend class Smi;
  friend class HeapObject;
  friend class Map;
  friend class String;
  friend class Script;
  friend class SharedFunctionInfo;
  friend class Code;
  friend class JSFunction;
  friend class OneByteString;
  friend class TwoByteString;
  friend class ConsString;
  friend class SlicedString;
  friend class ThinString;
  friend class HeapNumber;
  friend class JSObject;
  friend class JSError;
  friend class JSArray;
  friend class FixedArrayBase;
  friend class FixedArray;
  friend class FixedTypedArrayBase;
  friend class DescriptorArray;
  friend class NameDictionary;
  friend class Context;
  friend class ScopeInfo;
  friend class Oddball;
  friend class JSArrayBuffer;
  friend class JSArrayBufferView;
  friend class JSRegExp;
  friend class JSDate;
  friend class CodeMap;
  friend class llnode::FindJSObjectsVisitor;
  friend class llnode::FindObjectsCmd;
  friend class llnode::FindReferencesCmd;
  friend class llnode::node::constants::Environment;
};

#undef V8_VALUE_DEFAULT_METHODS

}  // namespace v8
}  // namespace llnode

#endif  // SRC_LLV8_H_
