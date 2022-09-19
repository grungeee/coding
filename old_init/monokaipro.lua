-- <===========< MONOKAIPRO >===========>

local colors = {
white = '#eefcfa',
black = '#000',
lightbrown = '#383539',
darkbrown= '#222023',
red = '#fa404f',
rose = '#ff6188',
orange = '#fc8a4a',
purple = '#ab9df2',
light_blue = '#78dce8',
green = '#a9dc76',
yellow = '#ffd866',
inactivegray = '#1C1E26',

} 

-- monokaipro.normal.a.bg = colors.rose
-- monokaipro.normal.a.fg = colors.darkbrown
-- monokaipro.normal.b.bg = colors.lightbrown
-- monokaipro.normal.b.fg = colors.white
-- monokaipro.normal.c.bg = colors.darkbrown
-- monokaipro.normal.c.fg = colors.white

-- monokaipro.insert.a.bg = colors.light_blue
-- monokaipro.insert.a.fg = colors.darkbrown
-- monokaipro.insert.b.bg = colors.lightbrown
-- monokaipro.insert.b.fg = colors.white
-- monokaipro.insert.c.bg = colors.darkbrown
-- monokaipro.insert.c.fg = colors.white

-- monokaipro.visual.a.bg = colors.purple
-- monokaipro.visual.a.fg = colors.darkbrown
-- monokaipro.visual.b.bg = colors.lightbrown
-- monokaipro.visual.b.fg = colors.white
-- onokaipro.visual.c.bg = colors.darkbrown
-- monokaipro.visual.c.fg = colors.white


-- monokaipro.command.a.bg = colors.yellow
-- monokaipro.command.a.fg = colors.darkbrown
-- monokaipro.command.b.bg = colors.lightbrown
-- monokaipro.command.b.fg = colors.white
-- monokaipro.command.c.bg = colors.darkbrown
-- monokaipro.command.c.fg = colors.white

 return {
 normal = {
   a = { bg = colors.rose, fg = colors.darkbrown, gui = 'bold' },
   b = { bg = colors.lightbrown, fg = colors.white },
   c = { bg = colors.darkbrown, fg = colors.white },
 },
 insert = {
   a = { bg = colors.light_blue, fg = colors.darkbrown, gui = 'bold' },
   b = { bg = colors.lightbrown, fg = colors.white },
   c = { bg = colors.darkbrown, fg = colors.white },
 },
 visual = {
   a = { bg = colors.purple, fg = colors.darkbrown, gui = 'bold' },
   b = { bg = colors.lightbrown, fg = colors.white },
   c = { bg = colors.darkbrown, fg = colors.white },
 },
 replace = {
   a = { bg = colors.red, fg = colors.darkbrown, gui = 'bold' },
   b = { bg = colors.lightbrown, fg = colors.white },
   c = { bg = colors.darkbrown, fg = colors.white },
 },
 command = {
   a = { bg = colors.yellow, fg = colors.darkbrown, gui = 'bold' },
   b = { bg = colors.lightbrown, fg = colors.white },
   c = { bg = colors.darkbrown, fg = colors.white },
 },
 inactive = {
   a = { bg = colors.inactivegray, fg = colors.lightbrown, gui = 'bold' },
   b = { bg = colors.inactivegray, fg = colors.lightbrown},
   c = { bg = colors.inactivegray, fg = colors.lightbrown},
 },


-- local monokaipro = require'lualine.themes.horizon'
-- Change the background of lualine_c section for normal mode
-- raplace && inactive


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
