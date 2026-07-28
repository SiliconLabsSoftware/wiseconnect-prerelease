local changeset = {}

-- Migrate NVM3 network/credential managers to Common Token Manager (flash) variants.
if slc.is_selected("nvm3_network_config_manager") or slc.is_provided("nvm3_network_config_manager") then
  table.insert(changeset, {
    ['component'] = 'nvm3_network_config_manager',
    ['action'] = 'remove',
    ['status'] = 'automatic',
    ['description'] =  [['Removing NVM3 network config manager from the project']]
  })
  if (not slc.is_selected("flash_network_config_manager")) and (not slc.is_provided("flash_network_config_manager")) then
    table.insert(changeset, {
      ['component'] = 'flash_network_config_manager',
      ['action'] = 'add',
      ['status'] = 'automatic',
      ['description'] =  [['Adding Common Token Manager (flash) network config manager to the project']]
    })
  end
end

if slc.is_selected("nvm3_credential_manager") or slc.is_provided("nvm3_credential_manager") then
  table.insert(changeset, {
    ['component'] = 'nvm3_credential_manager',
    ['action'] = 'remove',
    ['status'] = 'automatic',
    ['description'] =  [['Removing NVM3 credential manager from the project']]
  })
  if (not slc.is_selected("flash_credential_manager")) and (not slc.is_provided("flash_credential_manager")) then
    table.insert(changeset, {
      ['component'] = 'flash_credential_manager',
      ['action'] = 'add',
      ['status'] = 'automatic',
      ['description'] =  [['Adding Common Token Manager (flash) credential manager to the project']]
    })
  end
end

return changeset
