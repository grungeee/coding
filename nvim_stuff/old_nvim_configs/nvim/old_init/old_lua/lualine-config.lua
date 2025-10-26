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

local colors = {
white = '#eefcfa',
black = '#000',
lightbrown = '#383539',
darkgray = '#222023',
red = '#fa404f',
rose = '#ff6188',
orange = '#fc8a4a',
purple = '#ab9df2',
light_blue = '#78dce8',
green = '#a9dc76',
yellow = '#ffd866',
} 

local monokaipro = require'lualine.themes.horizon'
-- Change the background of lualine_c section for normal mode
monokaipro.normal.a.bg = colors.rose
monokaipro.normal.a.fg = colors.darkgray
monokaipro.insert.a.bg = colors.light_blue
monokaipro.insert.a.fg = colors.darkgray
monokaipro.visual.a.bg = colors.purple
monokaipro.visual.a.fg = colors.darkgray
monokaipro.command.a.bg = colors.yellow
monokaipro.command.a.fg = colors.darkgray
-- raplace && inactive

monokaipro.normal.b.bg = colors.lightbrown
monokaipro.normal.b.fg = colors.white
monokaipro.insert.b.bg = colors.lightbrown
monokaipro.insert.b.fg = colors.white
monokaipro.visual.b.bg = colors.lightbrown
monokaipro.visual.b.fg = colors.white
monokaipro.command.b.bg = colors.lightbrown
monokaipro.command.b.fg = colors.white

monokaipro.normal.c.bg = colors.darkgray
monokaipro.normal.c.fg = colors.white
monokaipro.insert.c.bg = colors.darkgray
monokaipro.insert.c.fg = colors.white
monokaipro.visual.c.bg = colors.darkgray
monokaipro.visual.c.fg = colors.white
monokaipro.command.c.bg = colors.darkgray
monokaipro.command.c.fg = colors.white

----------------------------------------------------------------------------------
require('lualine').setup {
  options = {
    icons_enabled = true,
    theme = monokaipro,
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
