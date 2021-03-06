/* <stig/rpc/var/get_type.h> 

   Get the type of a variant.

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

#include <memory>

#include <stig/rpc/type.h>
#include <stig/rpc/var.h>

/* Get the type of a variant. */
Stig::Rpc::TType::TPtr GetType(const Stig::Rpc::TVar &var);

