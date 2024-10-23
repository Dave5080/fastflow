/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __THREAD_MAPPER_HPP_
#define __THREAD_MAPPER_HPP_

/* ***************************************************************************
 *
 *  FastFlow is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License version 3 as
 *  published by the Free Software Foundation.
 *  Starting from version 3.0.1 FastFlow is dual licensed under the GNU LGPLv3
 *  or MIT License (https://github.com/ParaGroup/WindFlow/blob/vers3.x/LICENSE.MIT)
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 */

#include <stdlib.h>
#include <cstdlib>
#include <ff/config.hpp>
#include <ff/svector.hpp>
#include <ff/utils.hpp>
#include <ff/mapping_utils.hpp>
#include <vector>
#include <ff/affinity/parser.h>
#if defined(MAMMUT)
#include <mammut/mammut.hpp>
#endif


#if defined(FF_CUDA) 
#include <cuda.h>
#endif

namespace ff {

  class threadMapper{
    protected:
      threadMapper() { 
        char* env = ff::get_env("FF_AFFINITY");
        if(env){
          Lexer l(env);
;         Parser p(l);
          ff_cpu_set = p.parse_set();
        } else {
          cpu_set_t all_set;
          CPU_ZERO(&all_set);
          for(size_t i = 0; i < ff_numCores(); i++)
            CPU_SET(i, &all_set);
          FF_AFF_CPU_SETS def_set;
          def_set["all"] = all_set;
          ff_cpu_set = std::make_optional<FF_AFF_CPU_SETS>(def_set);
        }
      }

      std::optional<FF_AFF_CPU_SETS> ff_cpu_set;
      std::optional<std::function<cpu_set_t(std::optional<FF_AFF_CPU_SETS>, std::optional<std::string>&)> > scheduler = std::nullopt;
      void set_scheduler(std::function<cpu_set_t(std::optional<FF_AFF_CPU_SETS>, std::optional<std::string>&)> sched);
      cpu_set_t default_scheduler(std::optional<std::string>& node);
      static threadMapper *thm;
    public:
      static threadMapper* instance();
      cpu_set_t next(std::optional<std::string>& a);
  };

  threadMapper* threadMapper::thm = nullptr;
  
  cpu_set_t threadMapper::default_scheduler(std::optional<std::string>& tag){
    cpu_set_t res;
    if(!ff_cpu_set){
      CPU_ZERO(&res);
      return res;
    }
    if(!tag){
      static auto it = ff_cpu_set->begin();
      if(it == ff_cpu_set->end()) it = ff_cpu_set->begin();
      res = it->second;
      it++;
    } else {
      res = (*ff_cpu_set)[*tag];
    }
    return res;
  }


  threadMapper *threadMapper::instance() {
    if (thm == NULL)
      thm = new threadMapper();
    return thm;
  }
  cpu_set_t threadMapper::next(std::optional<std::string>& tag){
    if(scheduler) return (*scheduler)(ff_cpu_set, tag);
    auto res = default_scheduler(tag);
    if(ff_cpu_set){
      auto s = *ff_cpu_set;

    }
    return res;
  }
  void threadMapper::set_scheduler(std::function<cpu_set_t(std::optional<FF_AFF_CPU_SETS>, std::optional<std::string>&)> sched){
    scheduler = std::make_optional<std::function<cpu_set_t(std::optional<FF_AFF_CPU_SETS>, std::optional<std::string>&)>>(sched); 
  }
};
#endif /* __THREAD_MAPPER_HPP_ */
