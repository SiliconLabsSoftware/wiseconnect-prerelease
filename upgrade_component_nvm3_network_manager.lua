local changeset = {}
 
-- In wiseconnect 3.4.2, the NVM3 example projects did not explicitly include
-- network_manager. When upgrading directly to 4.0.0, this can surface as a
-- missing dependency for the NVM3 common-flash implementation.
--
-- For projects using sl_si91x_common_flash_nvm3 we must add the NVM3 variants
-- (nvm3_network_config_manager, nvm3_credential_manager), NOT the basic
-- variants. basic_* and nvm3_* are mutually exclusive (conflicts in SLCC);
-- adding both causes "Exclusivity Issue" at generation.
local nvm3_common_flash = slc.is_provided("sl_si91x_common_flash_nvm3")
 
if nvm3_common_flash and (not slc.is_selected("network_manager")) and (not slc.is_provided("network_manager")) then
  table.insert(changeset, {
    ['component'] = 'network_manager',
    ['action'] = 'add'
  })
end
 
-- Add NVM3 network config manager (required for NVM3 common-flash). Do NOT add
-- basic_network_config_manager here; it conflicts with nvm3_network_config_manager.
if nvm3_common_flash and (not slc.is_selected("nvm3_network_config_manager")) and (not slc.is_provided("nvm3_network_config_manager")) then
  table.insert(changeset, {
    ['component'] = 'nvm3_network_config_manager',
    ['action'] = 'add'
  })
end
 
-- nvm3_network_config_manager requires nvm3_credential_manager. Add it so
-- dependency is satisfied without pulling in basic_credential_manager.
if nvm3_common_flash and (not slc.is_selected("nvm3_credential_manager")) and (not slc.is_provided("nvm3_credential_manager")) then
  table.insert(changeset, {
    ['component'] = 'nvm3_credential_manager',
    ['action'] = 'add'
  })
end
 
-- Remove basic variants when present so they do not conflict with nvm3_*.
if nvm3_common_flash and slc.is_selected("basic_network_config_manager") then
  table.insert(changeset, {
    ['component'] = 'basic_network_config_manager',
    ['action'] = 'remove'
  })
end
if nvm3_common_flash and slc.is_selected("basic_credential_manager") then
  table.insert(changeset, {
    ['component'] = 'basic_credential_manager',
    ['action'] = 'remove'
  })
end
 
return changeset
 
