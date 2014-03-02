/* <stig/type/any.h> 

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

#include <stig/type/managed_type.h>

namespace Stig {

  namespace Type {

    class TAny : public TSingletonType<TAny> {
      NO_COPY_SEMANTICS(TAny);
      public:

      virtual ~TAny();

      private:
      TAny() {}

      virtual void Write(std::ostream &strm) const;

      friend class TSingletonType<TAny>;
    };  // TAny

  }  // Type

}  // Stig

