/* <stig/code_gen/keys.h> 

   TODO

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

#pragma once

#include <stig/code_gen/inline.h>

#include <stig/shared_enum.h>

namespace Stig {

  namespace CodeGen {

    class TKeys : public TInline {
      NO_COPY_SEMANTICS(TKeys);
      public:

      typedef std::vector<std::pair<TAddrDir, TInline::TPtr>> TAddrElems;
      typedef std::shared_ptr<const TKeys> TPtr;

      TKeys(const L0::TPackage *package,
            const Type::TType &ret_type,
            const Type::TType &val_type,
            TAddrElems &&addr_elems);

      void WriteExpr(TCppPrinter &out) const;

      /* Dependency graph */
      virtual void AppendDependsOn(std::unordered_set<TInline::TPtr> &dependency_set) const override {
        assert(this);
        for (const auto &iter : AddrElems) {
          dependency_set.insert(iter.second);
          iter.second->AppendDependsOn(dependency_set);
        }
      }

      private:
      TAddrElems AddrElems;

      Type::TType ValType;
    };

  } // CodeGen

} // Stig
