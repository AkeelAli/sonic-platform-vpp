/*
 * Copyright 2016 Microsoft, Inc.
 * Modifications copyright (c) 2023 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ContextConfig.h"

#include "swss/logger.h"

using namespace saivpp;

ContextConfig::ContextConfig(
        _In_ uint32_t guid,
        _In_ const std::string& name,
        _In_ const std::string& dbAsic):
    m_guid(guid),
    m_name(name),
    m_dbAsic(dbAsic)
{
    SWSS_LOG_ENTER();

    m_scc = std::make_shared<SwitchConfigContainer>();
}

ContextConfig::~ContextConfig()
{
    SWSS_LOG_ENTER();

    // empty
}

void ContextConfig::insert(
        _In_ std::shared_ptr<SwitchConfig> config)
{
    SWSS_LOG_ENTER();

    m_scc->insert(config);
}
