
local changeset = {}

-- Restores wireless/config components removed from sl_si91x_common_flash_nvm3 and
-- sl_si91x_littlefs_common_flash .slcc in 4.2.0. Invoked at 4.0.0 and 4.2.0 steps.
--
-- Upgrade paths to 4.2.0 (relevant slcu steps per jump):
--   4.0.0 -> 4.2.0 : 4.1.2 CTM, then this script (4.2.0 block)
--   4.1.x -> 4.2.0 : 4.1.2 CTM (if not already at 4.1.2), then this script (4.2.0 block)
--
-- | Case                         | Wireless restore | Config managers              |
-- |------------------------------|------------------|------------------------------|
-- | LittleFS common flash        | always pin       | see restore_config_managers  |
-- | NVM3 + PSA app               | skip             | skip (app owns components)   |
-- | NVM3, true legacy (no NM,    | pin              | flash_*; remove basic/nvm3_* |
-- |   no config managers)        |                  |                              |
-- | NVM3 otherwise               | pin              | see restore_config_managers  |
--
-- Use is_selected (not is_provided) for adds: autoselected deps are dropped after upgrade.
-- Use is_provided for trigger detection, network-stack detection, and removes on legacy NVM3 migrations.
local function uses_component(component)
  return slc.is_selected(component) or slc.is_provided(component)
end

local function uses_flash_config_managers()
  return uses_component("flash_network_config_manager")
      or uses_component("flash_credential_manager")
end

local function uses_nvm3_config_managers()
  return uses_component("nvm3_network_config_manager")
      or uses_component("nvm3_credential_manager")
end

local function uses_basic_config_managers()
  return uses_component("basic_network_config_manager")
      or uses_component("basic_credential_manager")
end

local common_flash_nvm3 = uses_component("sl_si91x_common_flash_nvm3")
local littlefs_common_flash = uses_component("sl_si91x_littlefs_common_flash")
local firmware_fallback = uses_component("sl_si91x_fw_fallback")
local is_psa_app = slc.is_selected("psa_crypto") or slc.is_selected("psa_driver")

local function add_if_not_selected(component)
  if not slc.is_selected(component) then
    table.insert(changeset, {
      ['component'] = component,
      ['action'] = 'add'
    })
  end
end

local function remove_if_present(component)
  if slc.is_selected(component) or slc.is_provided(component) then
    table.insert(changeset, {
      ['component'] = component,
      ['action'] = 'remove'
    })
  end
end

local function remove_component(component)
  table.insert(changeset, {
    ['component'] = component,
    ['action'] = 'remove'
  })
end

-- True if a concrete stack provider is already selected in the project.
local function has_selected_network_stack()
  return slc.is_selected("sl_si91x_internal_stack")
      or slc.is_selected("sl_si91x_lwip_stack")
      or slc.is_selected("sl_si91x_network_dual_stack")
end

local function restore_network_stack_provider()
  if has_selected_network_stack() then
    return
  end
  -- Preserve an autoselected alternate provider by pinning it before the
  -- upgrade drops autoselected dependencies. Prefer dual if it is visible.
  if slc.is_provided("sl_si91x_network_dual_stack") then
    add_if_not_selected("sl_si91x_network_dual_stack")
  elseif slc.is_provided("sl_si91x_lwip_stack") then
    add_if_not_selected("sl_si91x_lwip_stack")
  else
    add_if_not_selected("sl_si91x_internal_stack")
  end
end

local function has_available_alternate_network_stack()
  return uses_component("sl_si91x_lwip_stack")
      or uses_component("sl_si91x_network_dual_stack")
end

local function firmware_fallback_uses_network()
  if not firmware_fallback then
    return false
  end
  if uses_component("wifi") or uses_component("sl_si91x_wireless") then
    return true
  end
  if uses_component("network_manager") or uses_basic_config_managers() then
    return true
  end
  if uses_flash_config_managers() or uses_nvm3_config_managers() then
    return true
  end
  if uses_component("bsd_socket") or uses_component("bsd_socket_api") then
    return true
  end
  return uses_component("sl_si91x_socket")
      or uses_component("sl_si91x_asynchronous_socket")
end

local function restore_wireless_stack()
  add_if_not_selected("wifi")
  add_if_not_selected("sl_si91x_wireless")
  add_if_not_selected("network_manager")
  add_if_not_selected("wifi_resources")
  restore_network_stack_provider()
end

-- basic_*, flash_*, and nvm3_* managers conflict in pairs. nvm3_* is migrated to
-- flash_* by upgrade_wiseconnect_ctm.lua at the 4.1.2 step (runs before 4.2.0).
local function restore_config_managers()
  if uses_flash_config_managers() then
    add_if_not_selected("flash_network_config_manager")
    add_if_not_selected("flash_credential_manager")
    -- The LittleFS manifest schedules basic_network_config_manager in the same
    -- upgrade step. Remove it unconditionally because that scheduled add is not
    -- visible through is_selected/is_provided in this script snapshot.
    remove_component("basic_network_config_manager")
    remove_if_present("basic_credential_manager")
  elseif uses_nvm3_config_managers() then
    -- Upgrade scripts inspect the original project snapshot, so the 4.1.2 CTM
    -- changes may not be visible here during a 4.1.x -> 4.2.0 jump. Repeat the
    -- migration to guarantee that network_manager keeps a concrete provider.
    add_if_not_selected("flash_network_config_manager")
    add_if_not_selected("flash_credential_manager")
    -- Cancel the LittleFS manifest's same-step basic manager before adding
    -- the flash provider, just as in the already-flash branch above.
    remove_component("basic_network_config_manager")
    remove_if_present("basic_credential_manager")
    remove_if_present("nvm3_network_config_manager")
    remove_if_present("nvm3_credential_manager")
  else
    add_if_not_selected("basic_network_config_manager")
  end
end

local function network_manager_missing()
  return (not slc.is_selected("network_manager"))
      and (not slc.is_provided("network_manager"))
end

-- True pre-4.0 NVM3 projects only: no wireless stack and no config-manager choice yet.
local function should_migrate_nvm3_legacy_config_managers()
  if not network_manager_missing() then
    return false
  end
  return not uses_flash_config_managers()
      and not uses_nvm3_config_managers()
      and not uses_basic_config_managers()
end

local function migrate_nvm3_legacy_config_managers()
  add_if_not_selected("flash_network_config_manager")
  add_if_not_selected("flash_credential_manager")
  remove_if_present("basic_network_config_manager")
  remove_if_present("basic_credential_manager")
  remove_if_present("nvm3_network_config_manager")
  remove_if_present("nvm3_credential_manager")
end

if littlefs_common_flash then
  restore_wireless_stack()
  restore_config_managers()
end

if common_flash_nvm3 and not is_psa_app then
  restore_wireless_stack()

  if should_migrate_nvm3_legacy_config_managers() then
    migrate_nvm3_legacy_config_managers()
  else
    restore_config_managers()
  end
end

-- In 4.2.0, sl_si91x_wireless requires sl_si91x_network_stack whenever network_manager
-- is present. internal/lwip/dual all provide that API, so SLC will not autoselect the
-- historical recommend. Apps that omitted an explicit stack in 4.1 (notably
-- m4_updater_security) need internal_stack pinned. Restrict this compatibility fix to
-- the Si91x wireless backend; other backends share network/config managers but must not
-- receive a Si91x stack. The manager check avoids adding a stack to Si91x wifi-only
-- projects that never linked one. has_selected_network_stack leaves lwip/dual alone:
-- pinning internal_stack there would double-provide sl_si91x_network_stack.
local function needs_network_stack_provider()
  if uses_component("sl_siwx3xx_wireless") then
    return false
  end
  -- sl_si91x_wireless may only be scheduled later in the same upgrade. Use
  -- Si91x-specific common-flash and firmware-fallback components as backend
  -- signals too; m4_updater_security exposes firmware_fallback in its snapshot.
  local has_si91x_backend = uses_component("sl_si91x_wireless")
  if common_flash_nvm3 or littlefs_common_flash or firmware_fallback then
    has_si91x_backend = true
  end
  if not has_si91x_backend then
    return false
  end
  -- The fallback applications' network/config managers can also be scheduled
  -- later and therefore be absent from this snapshot. Their legacy bsd_socket
  -- selection is visible and identifies the variants that require a stack.
  local fallback_uses_network = firmware_fallback_uses_network()
  return fallback_uses_network
      or uses_component("network_manager")
      or uses_basic_config_managers()
      or uses_flash_config_managers()
end

-- The manifest pins internal_stack for every firmware-fallback project because
-- upgrade rules cannot express a compound trigger. Cancel it for the non-networked
-- updater and for fallback projects using a selected or provided alternate stack.
local remove_manifest_fallback_stack = false
if firmware_fallback
    and not slc.is_selected("sl_si91x_internal_stack")
    and not firmware_fallback_uses_network() then
  remove_manifest_fallback_stack = true
end
if firmware_fallback and has_available_alternate_network_stack() then
  remove_manifest_fallback_stack = true
end
if remove_manifest_fallback_stack then
  remove_component("sl_si91x_internal_stack")
  if firmware_fallback_uses_network() and has_available_alternate_network_stack() then
    restore_network_stack_provider()
  end
elseif needs_network_stack_provider()
    and not littlefs_common_flash
    and not (common_flash_nvm3 and not is_psa_app)
    and not firmware_fallback then
  restore_network_stack_provider()
end

return changeset