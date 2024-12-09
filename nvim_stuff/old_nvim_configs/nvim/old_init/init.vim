" <===================< Vim >==================>
source ~/.config/nvim/vim-plug/plugins.vim
source ~/.config/nvim/general/settings.vim
source ~/.config/nvim/keys/mappings.vim
source ~/.config/nvim/general/neovide-settings.vim


" source C:/Users/nikit/AppData/Local/nvim/vim-plug/startup_nvim.vim

" <===================< Lua >==================>
lua require'plug-colorizer-config' -- C:\Users\nikit\AppData\Local\nvim\lua\plug-colorizer-config.lua
lua require'nvim-treesitter.configs'.setup{highlight={enable=true}}
lua require'lualine-config' -- C:\Users\nikit\AppData\Local\nvim\lua\lualine-config.lua
lua require'dashboard-config' -- C:\Users\nikit\AppData\Local\nvim\lua\dashboard-config.lua



" <===================< TODO >==================>
" - setup treesitter: 
"   - more langueges 
"   - comments
"   - add mdoules:
"       - colored brackets
" - setup dashboard
" - edit current theme

