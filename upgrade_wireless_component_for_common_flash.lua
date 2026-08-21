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

local function has_network_stack()
  return uses_component("sl_si91x_network_stack")
      or uses_component("sl_si91x_internal_stack")
      or uses_component("sl_si91x_lwip_stack")
      or uses_component("sl_si91x_network_dual_stack")
end

local function restore_wireless_stack()
  add_if_not_selected("wifi")
  add_if_not_selected("sl_si91x_wireless")
  add_if_not_selected("network_manager")
  add_if_not_selected("wifi_resources")
  if not has_network_stack() then
    add_if_not_selected("sl_si91x_internal_stack")
  end
end

-- basic_*, flash_*, and nvm3_* managers conflict in pairs. nvm3_* is migrated to
-- flash_* by upgrade_wiseconnect_ctm.lua at the 4.1.2 step (runs before 4.2.0).
local function restore_config_managers()
  if uses_flash_config_managers() then
    add_if_not_selected("flash_network_config_manager")
    add_if_not_selected("flash_credential_manager")
  elseif uses_nvm3_config_managers() then
    return
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

return changeset
