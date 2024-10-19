/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*!
 *  \link
 *  \file mapper.hpp
 *  \ingroup shared_memory_fastflow
 *
 *  \brief This file contains the thread mapper definition used in FastFlow
 */

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
          Parser p(env);
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
        for(const auto& [k, v] : *ff_cpu_set)
          std::cout << set_to_str((*ff_cpu_set)[k]) << std::endl << std::endl;
      }

      std::optional<FF_AFF_CPU_SETS> ff_cpu_set;
      std::optional<std::function<cpu_set_t(std::optional<FF_AFF_CPU_SETS>)>> scheduler = std::nullopt;
      cpu_set_t default_scheduler();
      static threadMapper *thm;
    public:
      static threadMapper* instance();
      cpu_set_t next();
  };

  threadMapper* threadMapper::thm = nullptr;

  cpu_set_t threadMapper::default_scheduler(){
    cpu_set_t res;
    if(!ff_cpu_set){
      CPU_ZERO(&res);
      return res;
    }
    static auto it = ff_cpu_set->begin();
    if(it == ff_cpu_set->end()) it = ff_cpu_set->begin();
    cpu_set_t fres = it->second;
    it++;
    return fres;
  }


  threadMapper *threadMapper::instance() {
    if (thm == NULL)
      thm = new threadMapper();
    return thm;
  }
  cpu_set_t threadMapper::next(){
    if(scheduler) return (*scheduler)(ff_cpu_set);
    return default_scheduler();
  }
};
#endif /* __THREAD_MAPPER_HPP_ */
