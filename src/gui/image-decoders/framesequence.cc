/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "framesequence.h"
namespace cdroid{

int FrameSequence::mHeaderBytesRequired = 0;
FrameSequence::Registry* FrameSequence::mHead = nullptr;

FrameSequence::FrameSequence(cdroid::Context*ctx):mContext(ctx){
}

FrameSequence::Registry::Registry(const RegistryEntry& entry) {
    mImpl = entry;
    mNext = mHead;
    mHead = this;
    if (mHeaderBytesRequired < entry.requiredHeaderBytes) {
        mHeaderBytesRequired = entry.requiredHeaderBytes;
    }
}

const FrameSequence::RegistryEntry* FrameSequence::Registry::find(std::istream* stream) {
    Registry* registry = mHead;
    const off_t headerSize = mHeaderBytesRequired;
    char header[headerSize];
    stream->read(header, headerSize);
    stream->seekg(-headerSize,std::ios::cur);
    while (registry) {
        if (headerSize >= registry->mImpl.requiredHeaderBytes
                && registry->mImpl.checkHeader(header, headerSize)) {
            return &(registry->mImpl);
        }
        registry = registry->mNext;
    }
    return 0;
}

FrameSequence* FrameSequence::create(cdroid::Context*ctx,std::istream* stream) {
    const RegistryEntry* entry = Registry::find(stream);

    if (!entry) return NULL;
    FrameSequence* frameSequence = entry->createFrameSequence(ctx,stream);
    if (!frameSequence->getFrameCount() ||
            !frameSequence->getWidth() || !frameSequence->getHeight()) {
        // invalid contents, abort
        delete frameSequence;
        return NULL;
    }

    return frameSequence;
}

}/*endof namespace*/

