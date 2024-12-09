-- local lualine = require 'lualine'

-- -- Color table for highlights
-- local colors = {
--   bg = '#202328',
--   fg = '#bbc2cf',
--   yellow = '#ECBE7B',
--   cyan = '#008080',
--   darkblue = '#081633',
--   green = '#98be65',
--   orange = '#FF8800',
--   violet = '#a9a1e1',
--   magenta = '#c678dd',
--   blue = '#51afef',
--   red = '#ec5f67'
-- }

-- local conditions = {
--   buffer_not_empty = function() return vim.fn.empty(vim.fn.expand('%:t')) ~= 1 end,
--   hide_in_width = function() return vim.fn.winwidth(0) > 80 end,
--   check_git_workspace = function()
--     local filepath = vim.fn.expand('%:p:h')
--     local gitdir = vim.fn.finddir('.git', filepath .. ';')
--     return gitdir and #gitdir > 0 and #gitdir < #filepath
--   end
-- }

-- -- Config
-- local config = {
--   options = {
--     -- Disable sections and component separators
--     component_separators = "",
--     section_separators = "",
--     theme = {
--       -- We are going to use lualine_c an lualine_x as left and
--       -- right section. Both are highlighted by c theme .  So we
--       -- are just setting default looks o statusline
--       normal = {c = {fg = colors.fg, bg = colors.bg}},
--       inactive = {c = {fg = colors.fg, bg = colors.bg}}
--       -- REMAKE
--       }
--   },
--   sections = {
--     -- these are to remove the defaults
--     lualine_a = {},
--     lualine_b = {},
--     lualine_y = {},
--     lualine_z = {},
--     -- These will be filled later
--     lualine_c = {},
--     lualine_x = {}
--   },
--   inactive_sections = {
--     -- these are to remove the defaults
--     lualine_a = {},
--     lualine_v = {},
--     lualine_y = {},
--     lualine_z = {},
--     lualine_c = {},
--     lualine_x = {}
--   }
-- }

-- -- Inserts a component in lualine_c at left section
-- local function ins_left(component)
--   table.insert(config.sections.lualine_c, component)
-- end

-- -- Inserts a component in lualine_x ot right section
-- local function ins_right(component)
--   table.insert(config.sections.lualine_x, component)
-- end

-- ins_left {
--   function() return '▊' end,
--   color = {fg = colors.blue}, -- Sets highlighting of component
--   left_padding = 0 -- We don't need space before this
-- }

-- ins_left {
--   -- mode component
--   function()
--     -- auto change color according to neovims mode
--     local mode_color = {
--       n = colors.red,
--       i = colors.green,
--       v = colors.blue,
--       [''] = colors.blue,
--       V = colors.blue,
--       c = colors.magenta,
--       no = colors.red,
--       s = colors.orange,
--       S = colors.orange,
--       [''] = colors.orange,
--       ic = colors.yellow,
--       R = colors.violet,
--       Rv = colors.violet,
--       cv = colors.red,
--       ce = colors.red,
--       r = colors.cyan,
--       rm = colors.cyan,
--       ['r?'] = colors.cyan,
--       ['!'] = colors.red,
--       t = colors.red
--     }
--     vim.api.nvim_command(
--         'hi! LualineMode guifg=' .. mode_color[vim.fn.mode()] .. " guibg=" ..
--             colors.bg)
--     return ''
--   end,
--   color = "LualineMode",
--   left_padding = 0
-- }

-- ins_left {
--   -- filesize component
--   function()
--     local function format_file_size(file)
--       local size = vim.fn.getfsize(file)
--       if size <= 0 then return '' end
--       local sufixes = {'b', 'k', 'm', 'g'}
--       local i = 1
--       while size > 1024 do
--         size = size / 1024
--         i = i + 1
--       end
--       return string.format('%.1f%s', size, sufixes[i])
--     end
--     local file = vim.fn.expand('%:p')
--     if string.len(file) == 0 then return '' end
--     return format_file_size(file)
--   end,
--   condition = conditions.buffer_not_empty
-- }

-- ins_left {
--   'filename',
--   condition = conditions.buffer_not_empty,
--   color = {fg = colors.magenta, gui = 'bold'}
-- }

-- ins_left {'location'}

-- ins_left {'progress', color = {fg = colors.fg, gui = 'bold'}}

-- ins_left {
--   'diagnostics',
--   sources = {'nvim_diagnostic'},
--   symbols = {error = ' ', warn = ' ', info = ' '},
--   color_error = colors.red,
--   color_warn = colors.yellow,
--   color_info = colors.cyan
-- }

-- -- Insert mid section. You can make any number of sections in neovim :)
-- -- for lualine it's any number greater then 2
-- ins_left {function() return '%=' end}

-- ins_left {
--   -- Lsp server name .
--   function()
--     local msg = 'No Active Lsp'
--     local buf_ft = vim.api.nvim_buf_get_option(0, 'filetype')
--     local clients = vim.lsp.get_active_clients()
--     if next(clients) == nil then return msg end
--     for _, client in ipairs(clients) do
--       local filetypes = client.config.filetypes
--       if filetypes and vim.fn.index(filetypes, buf_ft) ~= -1 then
--         return client.name
--       end
--     end
--     return msg
--   end,
--   icon = '  ',
--   color = {fg = '#ffffff', gui = 'none'},
--   condition = conditions.hide_in_width
-- }

-- -- Add components to right sections
-- ins_right {
--   'o:encoding', -- option component same as &encoding in viml
--   upper = true, -- I'm not sure why it's upper case either ;)
--   condition = conditions.hide_in_width,
--   color = {fg = colors.green, gui = 'bold'}
-- }

-- ins_right {
--   'fileformat',
--   upper = true,
--   icons_enabled = false, -- I think icons are cool but Eviline doesn't have them. sigh
--   color = {fg = colors.green, gui = 'bold'}
-- }

-- ins_right {
--   'branch',
--   icon = '',
--   condition = conditions.check_git_workspace,
--   color = {fg = colors.violet, gui = 'bold'}
-- }

-- ins_right {
--   'diff',
--   -- Is it me or the symbol for modified us really weird
--   symbols = {added = ' ', modified = '柳 ', removed = ' '},
--   color_added = colors.green,
--   color_modified = colors.orange,
--   color_removed = colors.red,
--   condition = conditions.hide_in_width
-- }

-- ins_right {
--   function() return '▊' end,
--   color = {fg = colors.blue},
--   right_padding = 0
-- }

-- -- Now don't forget to initialize lualine
-- lualine.setup(config)

-----------------------------------------------------------------------------------
-----------------------------------------------------------------------------------
-----------------------------------------------------------------------------------
-- Example how to make a custom theme
-----------------------------------------------------------------------------------
-- Copyright (c) 2021 Jnhtr
-- MIT license, see LICENSE for more details.
-- stylua: ignore
-- local colors = {
-- black        = '#1c1e26',
-- white        = '#6C6F93',
-- red          = '#F43E5C',
-- green        = '#09F7A0',
-- blue         = '#25B2BC',
-- yellow       = '#F09383',
-- gray         = '#E95678',
-- darkgray     = '#1A1C23',
-- lightgray    = '#2E303E',
-- inactivegray = '#1C1E26',
--
--
-- return {
-- normal = {
--   a = { bg = colors.gray, fg = colors.black, gui = 'bold' },
--   b = { bg = colors.lightgray, fg = colors.white },
--   c = { bg = colors.darkgray, fg = colors.white },
-- },
-- insert = {
--   a = { bg = colors.blue, fg = colors.black, gui = 'bold' },
--   b = { bg = colors.lightgray, fg = colors.white },
--   c = { bg = colors.darkgray, fg = colors.white },
-- },
-- visual = {
--   a = { bg = colors.yellow, fg = colors.black, gui = 'bold' },
--   b = { bg = colors.lightgray, fg = colors.white },
--   c = { bg = colors.darkgray, fg = colors.white },
-- },
-- replace = {
--   a = { bg = colors.red, fg = colors.black, gui = 'bold' },
--   b = { bg = colors.lightgray, fg = colors.white },
--   c = { bg = colors.darkgray, fg = colors.white },
-- },
-- command = {
--   a = { bg = colors.green, fg = colors.black, gui = 'bold' },
--   b = { bg = colors.lightgray, fg = colors.white },
--   c = { bg = colors.darkgray, fg = colors.white },
-- },
-- inactive = {
--   a = { bg = colors.inactivegray, fg = colors.lightgray, gui = 'bold' },
--   b = { bg = colors.inactivegray, fg = colors.lightgray },
--   c = { bg = colors.inactivegray, fg = colors.lightgray },
-- },
--
---------------------------------------------------------------------------------

-- local colors = {
-- white = '#eefcfa',
-- black = '#000',
-- lightbrown = '#383539',
-- darkgray = '#222023',
-- red = '#fa404f',
-- rose = '#ff6188',
-- orange = '#fc8a4a',
-- purple = '#ab9df2',
-- light_blue = '#78dce8',
-- green = '#a9dc76',
-- yellow = '#ffd866',
-- } 

-- local monokaipro = require'lualine.themes.horizon'
-- -- Change the background of lualine_c section for normal mode
-- monokaipro.normal.a.bg = colors.rose
-- monokaipro.normal.a.fg = colors.darkgray
-- monokaipro.insert.a.bg = colors.light_blue
-- monokaipro.insert.a.fg = colors.darkgray
-- monokaipro.visual.a.bg = colors.purple
-- monokaipro.visual.a.fg = colors.darkgray
-- monokaipro.command.a.bg = colors.yellow
-- monokaipro.command.a.fg = colors.darkgray
-- -- raplace && inactive

-- monokaipro.normal.b.bg = colors.lightbrown
-- monokaipro.normal.b.fg = colors.white
-- monokaipro.insert.b.bg = colors.lightbrown
-- monokaipro.insert.b.fg = colors.white
-- monokaipro.visual.b.bg = colors.lightbrown
-- monokaipro.visual.b.fg = colors.white
-- monokaipro.command.b.bg = colors.lightbrown
-- monokaipro.command.b.fg = colors.white

-- monokaipro.normal.c.bg = colors.darkgray
-- monokaipro.normal.c.fg = colors.white
-- monokaipro.insert.c.bg = colors.darkgray
-- monokaipro.insert.c.fg = colors.white
-- monokaipro.visual.c.bg = colors.darkgray
-- monokaipro.visual.c.fg = colors.white
-- monokaipro.command.c.bg = colors.darkgray
-- monokaipro.command.c.fg = colors.white

local monokaipro = require'lualine.themes.monokaipro'

----------------------------------------------------------------------------------
require('lualine').setup {
  options = {
    icons_enabled = true,
    theme = monokaipro,
    -- theme = 'monokaipro',
    component_separators = { left = '', right = ''},
    section_separators = { left = '', right = ''},
    disabled_filetypes = {
      statusline = {},
      winbar = {},
    },
    ignore_focus = {},
    always_divide_middle = true,
    globalstatus = false,
    refresh = {
      statusline = 1000,
      tabline = 1000,
      winbar = 1000,
    }
  },
  sections = {
    lualine_a = {'mode'},
    lualine_b = {'branch', 'diff', 'diagnostics'},
    lualine_c = {'filename'},
    lualine_x = {'encoding', 'fileformat', 'filetype'},
    lualine_y = {'progress'},
    lualine_z = {'location'}
  },
  inactive_sections = {
    lualine_a = {},
    lualine_b = {},
    lualine_c = {'filename'},
    lualine_x = {'location'},
    lualine_y = {},
    lualine_z = {}
  },
  tabline = {},
  winbar = {},
  inactive_winbar = {},
  extensions = {}
}
