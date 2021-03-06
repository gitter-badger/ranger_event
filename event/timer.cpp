// Copyright (c) 2015, RangerUFO
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// * Neither the name of ranger_event nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "timer.hpp"
#include "dispatcher.hpp"
#include <event2/event.h>
#include <stdexcept>

namespace ranger { namespace event {

	timer::timer(dispatcher& disp)
	{
		_init(disp._event_base());
	}

	timer::~timer()
	{
		if (m_event)
			event_free(m_event);
	}

	namespace
	{

		void handle_expire(evutil_socket_t fd, short what, void* ctx)
		{
			auto tmr = static_cast<timer*>(ctx);
			auto& handler = tmr->get_event_handler();
			if (handler)
				handler(*tmr);
		}

	}

	void timer::_init(event_base* base)
	{
		m_event = event_new(base, -1, 0, handle_expire, this);
		if (!m_event)
			throw std::runtime_error("event create failed.");
	}

	void timer::_active(long sec, long usec)
	{
		if (sec > 0 || usec > 0)
		{
			timeval tv;
			tv.tv_sec = sec;
			tv.tv_usec = usec;

			event_add(m_event, &tv);
		}
	}

} }
