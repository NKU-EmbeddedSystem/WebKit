/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(APPLE_PAY)

#include <wtf/Optional.h>

#if USE(APPLE_INTERNAL_SDK)
#include <WebKitAdditions/ApplePayShippingMethodDataAdditions.h>
#endif

namespace WebCore {

struct ApplePayShippingMethodData {
#if defined(ApplePayShippingMethodDataAdditions_members)
    ApplePayShippingMethodDataAdditions_members
#endif

    template<class Encoder> void encode(Encoder&) const;
    template<class Decoder> static std::optional<ApplePayShippingMethodData> decode(Decoder&);

    template<class Decoder> WARN_UNUSED_RETURN bool decodeData(Decoder&);
};

template<class Encoder>
void ApplePayShippingMethodData::encode(Encoder& encoder) const
{
#if defined(ApplePayShippingMethodDataAdditions_encode)
    ApplePayShippingMethodDataAdditions_encode
#else
    UNUSED_PARAM(encoder);
#endif
}

template<class Decoder>
std::optional<ApplePayShippingMethodData> ApplePayShippingMethodData::decode(Decoder& decoder)
{
    ApplePayShippingMethodData result;
    if (!result.decodeData(decoder))
        return std::nullopt;
    return result;
}

template<class Decoder>
bool ApplePayShippingMethodData::decodeData(Decoder& decoder)
{
#if defined(ApplePayShippingMethodDataAdditions_decodeData)
    ApplePayShippingMethodDataAdditions_decodeData
#else
    UNUSED_PARAM(decoder);
#endif

    return true;
}

} // namespace WebCore

#endif // ENABLE(APPLE_PAY)
