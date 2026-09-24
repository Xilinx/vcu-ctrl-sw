// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "InputLoader.hpp"

struct HeifLoader final : public InputLoader
{
  explicit HeifLoader(AL_ECodec eCodec);

  uint32_t ReadStream(std::istream& ifFileStream, AL_TBuffer* pBufStream, uint8_t& uBufFlags) override;

private:
  void PrepareStream(std::istream& ifFileStream);

  AL_ECodec m_eCodec;
  std::vector<uint8_t> m_Stream;
  size_t m_zReadOffset = 0;
  bool m_bPrepared = false;
};
