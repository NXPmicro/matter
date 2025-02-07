/*
 *
 *    Copyright (c) 2021-2023 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <lib/support/logging/CHIPLogging.h>

#include "AppTask.h"
#include "CHIPDeviceManager.h"

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <app/util/af-types.h>
#include "PumpManager.h"

using namespace ::chip;
using namespace ::chip::app::Clusters;
/***** Function declarations *****/
void OnOnOffPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value);
void OnLevelControlPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value);

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & path, uint8_t type, uint16_t size, uint8_t * value)
{
	/*
    chip::DeviceManager::CHIPDeviceManagerCallbacks * cb =
        chip::DeviceManager::CHIPDeviceManager::GetInstance().GetCHIPDeviceManagerCallbacks();
    if (cb != nullptr)
    {
        // propagate event to device manager
        cb->PostAttributeChangeCallback(path.mEndpointId, path.mClusterId, path.mAttributeId, type, size, value);
    }
	*/
	EndpointId endpointId   = path.mEndpointId;
    ClusterId clusterId     = path.mClusterId;
    AttributeId attributeId = path.mAttributeId;

    ChipLogProgress(NotSpecified,
                    "PostAttributeChangeCallback - Cluster ID: '0x%04lx', EndPoint ID: '0x%02x', Attribute ID: '0x%04lx'",
                    clusterId, endpointId, attributeId);

    switch (clusterId)
    {
    case PumpConfigurationAndControl::Id:
        break;

    case OnOff::Id:
        OnOnOffPostAttributeChangeCallback(endpointId, attributeId, value);
        break;

    case LevelControl::Id:
        OnLevelControlPostAttributeChangeCallback(endpointId, attributeId, value);
        break;

    default:
        ChipLogProgress(NotSpecified, "Unhandled cluster ID: 0x%04lx", clusterId);
        break;
    }
}

void OnOnOffPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    BitMask<PumpConfigurationAndControl::PumpStatusBitmap> pumpStatus;

    VerifyOrExit(attributeId == OnOff::Attributes::OnOff::Id,
                 ChipLogError(NotSpecified, "Unhandled Attribute ID: '0x%04lx", attributeId));
    VerifyOrExit(endpointId == 1, ChipLogError(NotSpecified, "Unexpected EndPoint ID: `0x%02x'", endpointId));

    if (PumpMgr().IsStopped() && *value == true)
    {
        ChipLogProgress(Zcl, "[pump-app] PumpMgr().InitiateAction - START_ACTION");
        PumpMgr().InitiateAction(AppEvent::kEventType_None, PumpManager::START_ACTION);
    }
    else if (!PumpMgr().IsStopped() && *value == false)
    {
        ChipLogProgress(Zcl, "[pump-app] PumpMgr().InitiateAction - STOP_ACTION");
        PumpMgr().InitiateAction(AppEvent::kEventType_None, PumpManager::STOP_ACTION);
    }

    // Adjust PumpStatus attribute
    ChipLogProgress(Zcl, "[pump-app] Adjust PumpStatus attribute");
    PumpConfigurationAndControl::Attributes::PumpStatus::Get(PCC_CLUSTER_ENDPOINT, &pumpStatus);
    if (PumpMgr().IsStopped())
    {
        pumpStatus.Clear(PumpConfigurationAndControl::PumpStatusBitmap::kRunning);
    }
    else
    {
        pumpStatus.Set(PumpConfigurationAndControl::PumpStatusBitmap::kRunning);
    }
    PumpConfigurationAndControl::Attributes::PumpStatus::Set(PCC_CLUSTER_ENDPOINT, pumpStatus);

exit:
    return;
}

void OnLevelControlPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    VerifyOrExit(attributeId == LevelControl::Attributes::CurrentLevel::Id,
                 ChipLogError(NotSpecified, "Unhandled Attribute ID: '0x%04lx", attributeId));
    VerifyOrExit(endpointId == 1, ChipLogError(NotSpecified, "Unexpected EndPoint ID: `0x%02x'", endpointId));

    ChipLogProgress(Zcl, "[pump-app] Cluster LevelControl: attribute CurrentLevel set to %u", *value);

exit:
    return;
}
