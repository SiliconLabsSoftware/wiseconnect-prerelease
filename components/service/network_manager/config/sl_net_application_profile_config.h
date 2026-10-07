/***************************************************************************/ /**
 * @file sl_net_application_profile_config.h
 * @brief SL Net application profile compile-time selection (UC)
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#ifndef SL_NET_APPLICATION_PROFILE_CONFIG_H
#define SL_NET_APPLICATION_PROFILE_CONFIG_H

/* Profile identifiers used as UC option values (and by application code). */
#ifndef SL_NET_APP_PROFILE_NONE
#define SL_NET_APP_PROFILE_NONE 0
#endif
#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_SWITCH
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_SWITCH 1
#endif

// <<< Use Configuration Wizard in Context Menu >>>

// <h> SL Net Application Profile

// <o SL_NET_APP_PROFILE> Application profile
// <i> Selects which application profile is compiled into the build.
// <i> Runtime APIs apply the in-use or not-in-use config group for this profile.
// <SL_NET_APP_PROFILE_NONE=> None
// <SL_NET_APP_PROFILE_NEUTRAL_LESS_SWITCH=> Neutral-less Matter switch
// <i> Default: SL_NET_APP_PROFILE_NONE
#ifndef SL_NET_APP_PROFILE
#define SL_NET_APP_PROFILE SL_NET_APP_PROFILE_NONE
#endif

// </h>

// <<< end of configuration section >>>

#endif /* SL_NET_APPLICATION_PROFILE_CONFIG_H */
