/*
CmdDafDefault.h
!CHAOS
Created by CUGenerator

Copyright 2013 INFN, National Institute of Nuclear Physics
Licensed under the Apache License, Version 2.0 (the "License")
you may not use this file except in compliance with the License.
      You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef __DafnePresenter__CmdDafDefault_h__
#define __DafnePresenter__CmdDafDefault_h__


#include "AbstractDafnePresenterCommand.h"

#include <bitset>
namespace c_data = chaos::common::data;
namespace ccc_slow_command = chaos::cu::control_manager::slow_command;
namespace driver {
	namespace dafnepresenter {
		DEFINE_BATCH_COMMAND_CLASS(CmdDafDefault,AbstractDafnePresenterCommand) {
			//implemented handler
			uint8_t implementedHandler();
			//initial set handler
			void setHandler(c_data::CDataWrapper *data);
			//custom acquire handler
			void acquireHandler();
			//correlation and commit handler
			void ccHandler();
			//manage the timeout 
			bool timeoutHandler();


			private:
			uint64_t *p_timestamp;
			int32_t *p_dafne_status;
			double *p_i_ele;
			double *p_i_pos;
			double *p_nbunch_ele;
			double *p_nbunch_pos;
			int32_t *p_fill_pattern_ele;
			int32_t *p_fill_pattern_pos;
			int32_t *p_lifetime_ele;
			int32_t *p_lifetime_pos;
			double* p_sx_ele, *p_sy_ele;
			double* p_th_ele,*p_th_pos;
			double*  p_sx_pos, *p_sy_pos;
			double *p_rf;
			double  *p_VUGEL101, *p_VUGEL102, *p_VUGEL103;
			double  *p_VUGEL201, *p_VUGEL202, *p_VUGEL203;
			double  *p_VUGES101, *p_VUGES102, *p_VUGES103;
			double  *p_VUGES201, *p_VUGES202, *p_VUGES203;

			double  *p_VUGPL101, *p_VUGPL102, *p_VUGPL103;
			double  *p_VUGPL201, *p_VUGPL202, *p_VUGPL203;
			double  *p_VUGPS101, *p_VUGPS102, *p_VUGPS103;
			double  *p_VUGPS201, *p_VUGPS202, *p_VUGPS203;

			double *p_ty_ele, *p_ty_pos;
			double *p_R2_CCAL, *p_R2_BKG;
			double *p_Dead_TC;
			double *p_R1C_ele, *p_R1C_pos;
			double * p_lum_CCAL;


		};
	}
}
#endif
