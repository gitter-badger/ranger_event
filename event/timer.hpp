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

#ifndef RANGER_EVENT_TIMER_HPP
#define RANGER_EVENT_TIMER_HPP

#include <functional>
#include <chrono>

struct event;
struct event_base;

namespace ranger { namespace event {

	class dispatcher;

	class timer
	{
	public:
		using event_handler = std::function<void(timer&)>;

	public:
		explicit timer(dispatcher& disp);

		template <class T>
		timer(dispatcher& disp, T&& handler)
			: timer(disp)
		{
			m_event_handler = std::forward<T>(handler);
		}

		~timer();

		timer(const timer&) = delete;
		timer& operator = (const timer&) = delete;

		template <class T>
		void set_event_handler(T&& handler) { m_event_handler = std::forward<T>(handler); }
		const event_handler& get_event_handler() const { return m_event_handler; }

		template <class _rep, class _period>
		void active(const std::chrono::duration<_rep, _period>& dur)
		{
			auto sec = std::chrono::duration_cast<std::chrono::seconds>(dur);
			auto usec = std::chrono::duration_cast<std::chrono::microseconds>(dur - sec);
			_active(sec.count(), usec.count());
		}

		void close() { timer(std::move(*this)); }

	private:
		timer(timer&& rhs)
			: m_event(rhs.m_event)
			, m_event_handler(std::move(rhs.m_event_handler))
		{
			rhs.m_event = nullptr;
		}

		void _init(event_base* base);
		void _active(long sec, long usec);

	private:
		struct event* m_event;
		event_handler m_event_handler;
	};

} }

#endif	// RANGER_EVENT_TIMER_HPP
