/**
 * Copyright (c) 2014 Carnegie Mellon University. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following acknowledgments and disclaimers.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. The names "Carnegie Mellon University," "SEI" and/or "Software
 *    Engineering Institute" shall not be used to endorse or promote products
 *    derived from this software without prior written permission. For written
 *    permission, please contact permission@sei.cmu.edu.
 * 
 * 4. Products derived from this software may not be called "SEI" nor may "SEI"
 *    appear in their names without prior written permission of
 *    permission@sei.cmu.edu.
 * 
 * 5. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 * 
 *      This material is based upon work funded and supported by the Department
 *      of Defense under Contract No. FA8721-05-C-0003 with Carnegie Mellon
 *      University for the operation of the Software Engineering Institute, a
 *      federally funded research and development center. Any opinions,
 *      findings and conclusions or recommendations expressed in this material
 *      are those of the author(s) and do not necessarily reflect the views of
 *      the United States Department of Defense.
 * 
 *      NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
 *      INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
 *      UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
 *      IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
 *      FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
 *      OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES
 *      NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
 *      TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 * 
 *      This material has been approved for public release and unlimited
 *      distribution.
 **/

#include "MapeLoop.h"

typedef  madara::knowledge::KnowledgeRecord::Integer  Integer;

gams::controllers::MapeLoop::MapeLoop (
  madara::knowledge::KnowledgeBase & knowledge)
  : knowledge_ (knowledge)
{
  define_mape ();
}

gams::controllers::MapeLoop::~MapeLoop ()
{

}

void
gams::controllers::MapeLoop::define_analyze (
  madara::knowledge::KnowledgeRecord (*func) (
    madara::knowledge::FunctionArguments &,
    madara::knowledge::Variables &))
{
  // define the analyze function
  knowledge_.define_function ("analyze", func);
}

void gams::controllers::MapeLoop::define_execute (
  madara::knowledge::KnowledgeRecord (*func) (
    madara::knowledge::FunctionArguments &,
    madara::knowledge::Variables &))
{
  // define the execute function
  knowledge_.define_function ("execute", func);
}

void
gams::controllers::MapeLoop::define_mape (const std::string & loop)
{
  // define the mape loop via KaRL compilation
  mape_loop_ = knowledge_.compile (loop);
}

void
gams::controllers::MapeLoop::define_monitor (
  madara::knowledge::KnowledgeRecord (*func) (
    madara::knowledge::FunctionArguments &,
    madara::knowledge::Variables &))
{
  // define the monitor function
  knowledge_.define_function ("monitor", func);
}

void gams::controllers::MapeLoop::define_plan (
  madara::knowledge::KnowledgeRecord (*func) (
    madara::knowledge::FunctionArguments &,
    madara::knowledge::Variables &))
{
  // define the plan function
  knowledge_.define_function ("plan", func);
}

void
gams::controllers::MapeLoop::init_vars (
  madara::knowledge::KnowledgeBase & knowledge,
  const Integer & id,
  const Integer & processes)
{
  // initialize the agents, swarm, and self variables
  variables::init_vars (agents_, knowledge_, processes);
  swarm_.init_vars (knowledge);
  self_.init_vars (knowledge, id);
}

madara::knowledge::KnowledgeRecord
gams::controllers::MapeLoop::run (double period, double max_runtime)
{
  // initialize wait settings
  madara::knowledge::WaitSettings  settings;
  settings.max_wait_time = max_runtime;
  settings.poll_frequency = period;

  // wait for the max_runtime or for monitor, analyze, plan, or execute
  // to return non-zero
  return knowledge_.wait (mape_loop_, settings);
}
