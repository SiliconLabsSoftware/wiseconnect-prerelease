local changeset = {}

-- In wiseconnect 3.4.2, the NVM3 example projects did not explicitly include
-- network_manager. When upgrading directly to 4.0.0, this can surface as a
-- missing dependency for the NVM3 common-flash implementation.
--
-- Add network_manager only when the Si91x common-flash NVM3 implementation is
-- present in the project and network_manager isn't already selected.
if slc.is_provided("sl_si91x_common_flash_nvm3")
   and (not slc.is_selected("network_manager"))
   and (not slc.is_provided("network_manager")) then
  table.insert(changeset, {
    ['component'] = 'network_manager',
    ['action'] = 'add'
  })
end

if slc.is_provided("sl_si91x_common_flash_nvm3")
   and (not slc.is_selected("basic_network_config_manager"))
   and (not slc.is_provided("basic_network_config_manager")) then
  table.insert(changeset, {
    ['component'] = 'basic_network_config_manager',
    ['action'] = 'add'
  })
end

return changeset
