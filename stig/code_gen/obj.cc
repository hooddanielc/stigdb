/* <stig/code_gen/obj.cc> 

   Implements <stig/code_gen/object.h>

   Copyright 2010-2014 Tagged
   
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
   
     http://www.apache.org/licenses/LICENSE-2.0
   
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#include <stig/code_gen/obj.h>

#include <sstream>

#include <base/not_implemented_error.h>
#include <base/split.h>
#include <base/time_maps.h>
#include <stig/code_gen/util.h>
#include <stig/type.h>
#include <stig/type/gen_code.h>
#include <stig/type/object_collector.h>

using namespace std;
using namespace std::placeholders;
using namespace Stig;
using namespace Stig::CodeGen;
using namespace Stig::Type;

TObjCtor::TObjCtor(const L0::TPackage *package, const Type::TType &type, TArgs &&args) : TInline(package, type), Args(args) {}

void TObjCtor::WriteExpr(TCppPrinter &out) const {
  assert(&out);

  out << GetReturnType() << '(';
  Base::Join(", ", Args, [](const TArgs::value_type &it, TCppPrinter &out){
    out << it.second;
  }, out) << ')';
}


void Stig::CodeGen::GenObjComparison(const Type::TType &, const Type::TType &, TCppPrinter &) {
  throw Base::TNotImplementedError(HERE);
}

/* a < that.a || (a == that.a && (b < that.b || (b == that.b && c < that.c)) ) */
void WriteLessExpr(TCppPrinter &out, Type::TObjElems::const_iterator iter, const Type::TObjElems::const_iterator &iter_end) {
  if (iter != iter_end) {
    Type::TObjElems::const_iterator next = iter; ++next;
    out << "(Rt::MatchLess(V" << iter->first << ", that.V" << iter->first << ")";
    if (next != iter_end) {
      out << " || (Rt::Match(V" << iter->first << ", that.V" << iter->first << ") && ";
      WriteLessExpr(out, next, iter_end);
      out << ")";
    }
    out << ")";
  }
}

void Stig::CodeGen::GenObjHeader(const Jhm::TAbsBase &out_dir, const Type::TType &obj_type) {
  assert(&out_dir);
  assert(&obj_type);

  auto obj_name = obj_type.GetMangledName();

  // Check for TimePnt and TimeDiff structural types so we don't regenerate them
  const Type::TObj *result = obj_type.TryAs<Type::TObj>();
  if (result) {
    if (Base::Chrono::IsTimeObj(result)) {
      return;
    }
  }

  Jhm::TAbsPath path(out_dir, Jhm::TRelPath(Jhm::TNamespace(""), Jhm::TName(obj_name, {"h"})));
  auto obj_class_name = "TObj" + obj_name;
  auto obj_core_type = obj_type.As<Type::TObj>();
  TCppPrinter out(path.AsStr());
  out << "/* <" << path.GetRelPath() << ">" << Eol
      << Eol
      << "   This file was auto-generated by the Stig compiler. */" << Eol
      << Eol
      << "#pragma once" << Eol
      << Eol
      << "#include <cassert>" << Eol
      << Eol
      << "#include <stig/rt/tuple.h>" << Eol // NOTE: This is only needed when the object contains an addr.
      << "#include <stig/rt/containers.h>" << Eol
      << "#include <stig/rt/obj.h>" << Eol
      << "#include <stig/type/obj.h>" << Eol
      << "#include <stig/var/impl.h>" << Eol;

  /* EXTRA */ {
    unordered_set<TType> obj_set;
    CollectObjects(obj_type, obj_set);
    assert(obj_set.count(obj_type));
    obj_set.erase(obj_type);
    assert(!obj_set.count(obj_type));
    if(obj_set.size()) {
      out << Eol
          << "/* Needed objects */" << Eol;
    }
    All(obj_set, bind(GenObjInclude, _1, ref(out)));
  }

  out << Eol;

  /* namespace Stig::Rt */ {
  TNamespacePrinter nsp(vector<string>{"Stig", "Rt"}, out);
    /* namespace Objects */ {
      TNamespacePrinter nsp(vector<string>{"Objects"}, out);
      out << "class " << obj_class_name << " : public TObj {" << Eol;
      /* Class body */ {
        TIndent indent(out);
        out << "public:" << Eol;
        /* TODO: It would be nice not to have a default ctor, but dictionary and set mandate it. */
        if(!obj_core_type->GetElems().empty()) {
          out << obj_class_name << "() {}" << Eol
              << Eol;
        }
        /* TODO: Should really write a generic object ctor printer */
        out << obj_class_name << "(const TDynamicMembers &m)";
        if(!obj_core_type->GetElems().empty()) {
          out << " : ";
        }
        Base::Join(", ", obj_core_type->GetElems(), [](const TObj::TElems::value_type &it, TCppPrinter &out) {
          out << 'V' << it.first << "(Sabot::AsNative<" << it.second << ">(*Sabot::State::TAny::TWrapper(m.at(\"" << it.first << "\").GetState(alloca(Sabot::State::GetMaxStateSize())))))";
        }, out);
        out << " {}" << Eol
            << obj_class_name << '(';

        Base::Join(", ", obj_core_type->GetElems(), [](const TObj::TElems::value_type &it, TCppPrinter &out) {
          //Two lower case v's, because there are some C++ keywords that start with one lower case 'v', and those should still
          //be valid object member names.
          //TODO: This manual prefixing ugly.
          out << "const " << it.second <<" &vv" << it.first;
        }, out);
        out << ")";
        if(!obj_core_type->GetElems().empty()) {
          out << " : ";
        }
        Base::Join(", ", obj_core_type->GetElems(), [](const TObj::TElems::value_type &it, TCppPrinter &out) {
          out << 'V' << it.first << "(vv" << it.first << ')';
        }, out);
        out << " {}" << Eol

        /* TODO: Move Ctor? */
            << obj_class_name << "(const " << obj_class_name << " &that)";
        if(!obj_core_type->GetElems().empty()) {
          out << " : ";
        }
        Base::Join(", ", obj_core_type->GetElems(), [](const TObj::TElems::value_type &it, TCppPrinter &out) {
          out << 'V' << it.first << "(that.V" << it.first << ')';
        }, out);
        out << " {}" << Eol
            << Eol
        #if 0
            << "Var::TVar AsVar() const final {" << Eol
            << "  assert(this);" << Eol
            << "  return Var::TVar::Obj(TDynamicMembers{";
        Base::Join(", ", obj_core_type->GetElems(), [](const TObj::TElems::value_type &it, TCppPrinter &out) {
          out << "{ \"" << it.first << "\", Var::TVar(V"<< it.first << ")}";
        }, out);
        out << "});" << Eol
            << '}' << Eol
            << Eol
            #endif
        /* TODO: Should really write a generic object member function printer */
        /* helper functions (hash, equality, getters, etc.) */
        //TODO: Meta
            << "size_t GetHash() const {" << Eol
            << "  assert(this);" << Eol
            << "  return  ";
        /* TODO: Better hash function. */
        if(obj_core_type->GetElems().empty()) {
          out << "0";
        }
        Base::Join(" ^ ", obj_core_type->GetElems(), [](const TObj::TElems::value_type &it, TCppPrinter &out) {
          out << "std::hash<" << it.second << ">()(V" << it.first << ')';
        }, out);
        out << ';' << Eol
            << '}' << Eol
            << Eol
            << (Type::HasOptional(obj_type) ? "TOpt<bool>" : "bool") << " EqEq(const " << obj_class_name
            << " &that) const {" << Eol
            << "  assert(this);" << Eol
            << "  assert(&that);" << Eol
            << "  return ";
        for (const auto &elem : obj_core_type->GetElems()) {
          out << "Rt::And(Rt::EqEq(V" << elem.first << ", that.V" << elem.first << "), ";
        }
        out << "true" << string(obj_core_type->GetElems().size(), ')') << ';' << Eol
            << '}' << Eol
            << Eol
            << Eol
            << "bool Match(const " << obj_class_name << " &that) const {" << Eol
            << "  assert(this);" << Eol
            << "  assert(&that);" << Eol
            << "  return ";
        if(obj_core_type->GetElems().empty()) {
          out << "true";
        }
        Base::Join(" && ", obj_core_type->GetElems(), [](const TObj::TElems::value_type &it, TCppPrinter &out) {
          out << "Rt::Match(V" << it.first << ", that.V" << it.first << ')';
        }, out);
        out << ';' << Eol
            << '}' << Eol;
        /* MatchLess */
        out << "bool MatchLess(const " << obj_class_name << " &that) const {" << Eol
            << "  assert(this);" << Eol
            << "  assert(&that);" << Eol
            << "  return ";
        if(obj_core_type->GetElems().empty()) {
          out << "false";
        } else {
          /* a < that.a || (a == that.a && (b < that.b || (b == that.b && c < that.c)) ) */
          WriteLessExpr(out, obj_core_type->GetElems().begin(), obj_core_type->GetElems().end());
        }
        out << ';' << Eol
            << '}' << Eol;
        /* End MatchLess */
        out << Eol
            << (Type::HasOptional(obj_type) ? "TOpt<bool>" : "bool") << " Neq(const " << obj_class_name
            << " &that) const {" << Eol
            << "  assert(this);" << Eol
            << "  assert(&that);" << Eol
            << "  return ";
        for (const auto &elem : obj_core_type->GetElems()) {
          out << "Rt::Or(Rt::Neq(V" << elem.first << ", that.V" << elem.first << "), ";
        }
        out << "false" << string(obj_core_type->GetElems().size(), ')') << ';' << Eol
            << '}' << Eol
            << Eol;
        for(auto &it: obj_core_type->GetElems()) {
          //NOTE: We prefix variables with 'V' here so that a variable name will never conflict with a C++ magic keyword.
          out << it.second <<" GetV" << it.first << "() const {" << Eol
              << "  assert(this);" << Eol
              << "  return V" << it.first << ';' << Eol
              << '}' << Eol;
        }

        out << Eol
            << "private:" << Eol;

        for(auto &it: obj_core_type->GetElems()) {
          out << "typedef " << it.second << " TVTV" << it.first << ";" << Eol;
          //NOTE: We prefix variables with 'V' here so that a variable name will never conflict with a C++ magic keyword.
          out << it.second <<" V" << it.first << ";" << Eol;
        }

        out << "friend class Stig::Sabot::TToNativeVisitor<" << obj_class_name << ">;" << Eol;

      } // Class body
      out << "}; // " << obj_class_name  << Eol;
    } // namespace Objects

    out << Eol
        << "template <>" << Eol
        << "struct EqEqStruct<Objects::" << obj_class_name << ", Objects::" << obj_class_name << "> {" << Eol
        << "  static " << (Type::HasOptional(obj_type) ? "TOpt<bool>" : "bool") << " Do(const Objects::" << obj_class_name
        << " &lhs, const Objects::" << obj_class_name << " &rhs) {" << Eol
        << "    return lhs.EqEq(rhs);" << Eol
        << "  }" << Eol
        << "}; // EqEqStruct<Objects::" << obj_class_name << ", Objects::" << obj_class_name << '>' << Eol
        << Eol
        << "template <>" << Eol
        << "struct NeqStruct<Objects::" << obj_class_name << ", Objects::" << obj_class_name << "> {" << Eol
        << "  static " << (Type::HasOptional(obj_type) ? "TOpt<bool>" : "bool") << " Do(const Objects::" << obj_class_name
        << " &lhs, const Objects::" << obj_class_name << " &rhs) {" << Eol
        << "    return lhs.Neq(rhs);" << Eol
        << "  }" << Eol
        << "}; // NeqStruct<Objects::" << obj_class_name << ", Objects::" << obj_class_name << '>' << Eol
        << Eol;



    out << "template <>" << Eol
        << "inline bool Match(const Objects::" << obj_class_name << " &lhs, const Objects::" << obj_class_name << " &rhs) {" << Eol
        << "  return lhs.Match(rhs);" << Eol
        << '}' << Eol;

    out << "template <>" << Eol
        << "inline bool MatchLess(const Objects::" << obj_class_name << " &lhs, const Objects::" << obj_class_name << " &rhs) {" << Eol
        << "  return lhs.MatchLess(rhs);" << Eol
        << '}' << Eol;

  } // namespace Stig::Rt

  out << Eol;

  /* namespace Stig::Type */ {
    TNamespacePrinter nsp(vector<string>{"Stig", "Type"}, out);
    out << "template <>" << Eol
        << "struct TDt<Rt::Objects::" << obj_class_name << "> {" << Eol
        << Eol
        << "  static TType GetType() {" << Eol
        << "    return TObj::Get({";
    Base::Join(", ", obj_core_type->GetElems(), [](const TObj::TElems::value_type &it, TCppPrinter &out) {
      out << "{\"" << it.first << "\", ";
      Type::GenCode(out.GetOstream(), it.second);
      out << '}';
    }, out);
    out << "});" << Eol
        << "  }" << Eol
        << Eol
        << "};" << Eol;
  } // namespace Stig::Type

  out << Eol;

  /* Namespace */ {
    out << "namespace std {" << Eol;
    TIndent indent(out);
    out << "template<>" << Eol
        << "struct hash<Stig::Rt::Objects::" << obj_class_name << "> {" << Eol << Eol
        << "  typedef size_t return_type;" << Eol
        << "  typedef Stig::Rt::Objects::" << obj_class_name << " argument_type;" << Eol << Eol
        << "  size_t operator()(const argument_type &obj) const {" << Eol
        << "    return  obj.GetHash();" << Eol
        << "  }" << Eol << Eol
        << "}; // hash<Stig::Rt::Objects::" << obj_class_name << '>' << Eol;
  }
  out << "} // std" << Eol;

  #if 0
  out << "namespace Stig {" << Eol;
  /* Namespace Stig */ {
    TIndent indent(out);
    out << "namespace Sabot {" << Eol << Eol;
    /* Namespace Sabot */ {
    TIndent indent(out);
    out << "template <>" << Eol
        << "class TToNativeVisitor<Stig::Rt::Objects::" << obj_class_name << "> final" << Eol;
      /* Class */ {
        TIndent indent(out);
        out << ": public TStateVisitor {" << Eol
            << "NO_COPY_SEMANTICS(TToNativeVisitor);" << Eol
            << "public:" << Eol
            << "/* TODO */" << Eol
            << "TToNativeVisitor(Stig::Rt::Objects::" << obj_class_name << " &out) : Out(out) {}" << Eol
            << "/* Overrides. */" << Eol
            << "virtual void operator()(const State::TFree &/*state*/) const override       { throw; }" << Eol
            << "virtual void operator()(const State::TTombstone &/*state*/) const override  { throw; }" << Eol
            << "virtual void operator()(const State::TVoid &/*state*/) const override       { throw; }" << Eol
            << "virtual void operator()(const State::TInt8 &/*state*/) const override       { throw; }" << Eol
            << "virtual void operator()(const State::TInt16 &/*state*/) const override      { throw; }" << Eol
            << "virtual void operator()(const State::TInt32 &/*state*/) const override      { throw; }" << Eol
            << "virtual void operator()(const State::TInt64 &/*state*/) const override      { throw; }" << Eol
            << "virtual void operator()(const State::TUInt8 &/*state*/) const override      { throw; }" << Eol
            << "virtual void operator()(const State::TUInt16 &/*state*/) const override     { throw; }" << Eol
            << "virtual void operator()(const State::TUInt32 &/*state*/) const override     { throw; }" << Eol
            << "virtual void operator()(const State::TUInt64 &/*state*/) const override     { throw; }" << Eol
            << "virtual void operator()(const State::TBool &/*state*/) const override       { throw; }" << Eol
            << "virtual void operator()(const State::TChar &/*state*/) const override       { throw; }" << Eol
            << "virtual void operator()(const State::TFloat &/*state*/) const override      { throw; }" << Eol
            << "virtual void operator()(const State::TDouble &/*state*/) const override     { throw; }" << Eol
            << "virtual void operator()(const State::TDuration &/*state*/) const override   { throw; }" << Eol
            << "virtual void operator()(const State::TTimePoint &/*state*/) const override  { throw; }" << Eol
            << "virtual void operator()(const State::TUuid &/*state*/) const override       { throw; }" << Eol
            << "virtual void operator()(const State::TBlob &/*state*/) const override       { throw; }" << Eol
            << "virtual void operator()(const State::TStr &/*state*/) const override        { throw; }" << Eol
            << "virtual void operator()(const State::TDesc &/*state*/) const override       { throw; }" << Eol
            << "virtual void operator()(const State::TOpt &/*state*/) const override        { throw; }" << Eol
            << "virtual void operator()(const State::TSet &/*state*/) const override        { throw; }" << Eol
            << "virtual void operator()(const State::TVector &/*state*/) const override     { throw; }" << Eol
            << "virtual void operator()(const State::TMap &/*state*/) const override        { throw; }" << Eol
            << "virtual void operator()(const State::TRecord &state) const override         { " << Eol;


        /* Func Impl */ {
          TIndent indent(out);
          out << "// get the record's type" << Eol
              << "void *record_type_alloc = alloca(Type::GetMaxTypeSize());" << Eol
              << "Type::TRecord::TWrapper record_type(state.GetRecordType(record_type_alloc));" << Eol
              << "// pin the record's type" << Eol
              << "void *record_type_pin_alloc = alloca(Type::GetMaxTypePinSize());" << Eol
              << "Type::TRecord::TPin::TWrapper record_type_pin(record_type->Pin(record_type_pin_alloc));" << Eol
              << "// pin the record's state" << Eol
              << "void *record_state_pin_alloc = alloca(State::GetMaxStatePinSize());" << Eol
              << "State::TRecord::TPin::TWrapper record_state_pin(state.Pin(record_state_pin_alloc));" << Eol
              << "// iterate over the record sabot (type and state, in parallel)" << Eol
              << "std::string elem_name;" << Eol
              << "void" << Eol
              << "    *elem_type_alloc  = alloca(Sabot::Type::GetMaxTypeSize())," << Eol
              << "    *elem_state_alloc = alloca(Sabot::State::GetMaxStateSize());" << Eol
              << "for (size_t elem_idx = 0; elem_idx < record_type_pin->GetElemCount(); ++elem_idx) {" << Eol
              << "  switch(elem_idx) {" << Eol;
          size_t idx = 0;
          for (const auto &elem : obj_core_type->GetElems()) {
            out << "    case " << idx << ": {" << Eol
                << "      Out.V" << elem.first << " = AsNative<" << elem.second << ">(*State::TAny::TWrapper(record_state_pin->NewElem(elem_idx, elem_state_alloc)));" << Eol
                << "      break;" << Eol
                << "    }" << Eol;
            ++idx;
          }

          #if 0
          out << "  Type::TAny::TWrapper elem_type(record_type_pin->NewElem(elem_idx, elem_name, elem_type_alloc));" << Eol
              << "  const auto *native_elem = Native::Record<TVal>::TryGetElem(elem_name.c_str());" << Eol
              << "  if (native_elem) {" << Eol
              << "    State::TAny::TWrapper elem_state(record_state_pin->NewElem(elem_idx, elem_state_alloc));" << Eol
              << "    native_elem->SetVal(Out, *elem_state);" << Eol
              << "  }" << Eol
          #endif

          out << "}" << Eol;
        }



        out << "  }" << Eol
            << "}" << Eol
            << "virtual void operator()(const State::TTuple &/*state*/) const override      { throw; }" << Eol
            << "private:" << Eol
            << "Stig::Rt::Objects::" << obj_class_name << " &Out;" << Eol;
      }
      out << "};  // TToNativeVisitor" << Eol;
    }
    out << "}" << Eol;
  }
  out << "}" << Eol;
  #endif

  for (const auto &elem : obj_core_type->GetElems()) {
    out << "RECORD_ELEM_WITH_FIELD_NAME(Stig::Rt::Objects::" << obj_class_name << ", Stig::Rt::Objects::" << obj_class_name << "::TVTV" << elem.first << ", V" << elem.first << ", " << elem.first << ");" << Eol;
  }

}

void Stig::CodeGen::GenObjInclude(const Type::TType &obj_type, TCppPrinter &out) {
  assert(&obj_type);
  assert(&out);

  // Check for TimePnt and TimeDiff structural types so we don't regenerate them
  const Type::TObj *result = obj_type.TryAs<Type::TObj>();
  if (result) {
    if (Base::Chrono::IsTimeObj(result)) {
      return;
    }
  }

  //TODO: Assert that obj_type is indeed an object type
  out << "#include <stig/rt/objects/" << obj_type.GetMangledName() << ".h>" << Eol;

}
