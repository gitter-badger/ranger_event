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

#include "token_bucket_cfg.hpp"
#include <event2/bufferevent.h>
#include <stdexcept>

namespace ranger { namespace event {

	token_bucket_cfg::~token_bucket_cfg()
	{
		if (m_cfg)
		{
			ev_token_bucket_cfg_free(m_cfg);
		}
	}

	std::shared_ptr<token_bucket_cfg> token_bucket_cfg::create(size_t read_rate, size_t read_burst, size_t write_rate, size_t write_burst, float period /* = 0.0f */)
	{
		return std::make_shared<token_bucket_cfg>(read_rate, read_burst, write_rate, write_burst, period);
	}

	token_bucket_cfg::token_bucket_cfg(size_t read_rate, size_t read_burst, size_t write_rate, size_t write_burst, float period)
	{
		if (period > 0.0f)
		{
			timeval tv;
			tv.tv_sec = static_cast<long>(period);
			tv.tv_usec = static_cast<long>((period - tv.tv_sec) * 1e6);

			m_cfg = ev_token_bucket_cfg_new(read_rate, read_burst, write_rate, write_burst, &tv);
		}
		else
		{
			m_cfg = ev_token_bucket_cfg_new(read_rate, read_burst, write_rate, write_burst, nullptr);
		}

		if (!m_cfg)
		{
			throw std::runtime_error("ev_token_bucket_cfg_new call failed.");
		}
	}

} }