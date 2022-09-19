language en_US

"  :lua require('dashboard-config') 

"" * Neovide settings
  "  Put anything you want to happen only in Neovide here
if exists("g:neovide")
let g:neovide_refresh_rate=60
let g:neovide_transparency=1
let g:neovide_floating_blur_amount_x = 2.0
let g:neovide_floating_blur_amount_y = 2.0
let g:neovide_scroll_animation_length = 0.3
let g:neovide_fullscreen=v:true
let g:neovide_remember_window_size = v:true
let g:neovide_cursor_vfx_mode = "railgun"
endif

call plug#begin()
Plug 'tpope/vim-commentary'
Plug 'tpope/vim-fugitive'
Plug 'sheerun/vim-polyglot'
Plug 'neoclide/coc.nvim', {'branch': 'release'}
Plug 'mattn/emmet-vim'
Plug 'vim-airline/vim-airline'
Plug 'vim-airline/vim-airline-themes'
Plug 'scrooloose/nerdtree'
Plug 'jistr/vim-nerdtree-tabs'

Plug 'glepnir/dashboard-nvim'
Plug 'https://gitlab.com/__tpb/monokai-pro.nvim' "monokaipro
Plug 'phanviet/vim-monokai-pro' "monokai_pro
Plug 'gruvbox-community/gruvbox'
call plug#end()


nnoremap <c-f> :NERDTreeFocus<CR>
nnoremap <c-n> :NERDTree<CR>
nnoremap <c-t> :NERDTreeToggle<CR>

inoremap jk <c-c>
inoremap kj <c-c>


"" Visual Settings
colorscheme monokaipro
let g:lightline = {
      \ 'colorscheme': 'monokaipro',
      \ }

set guifont=Cascadia\ Code\:h14

syntax on
set ruler
set number

"" Tabs. May be overridden by autocmd rules
set tabstop=4
set softtabstop=0
set shiftwidth=4
set expandtab

set backspace=indent,eol,start
set scrolloff=8
set signcolumn=yes
set cmdheight=2

"" Searching
set hlsearch
set incsearch
set ignorecase
set smartcase

" * Dashboard Settings

"  local home = os.getenv('HOME')
"  local db = require('dashboard')
"  -- macos
"  db.preview_command = 'cat | lolcat -F 0.3'
"  -- linux
"  db.preview_command = 'ueberzug'
"  --
"  db.preview_file_path = home .. '/.config/nvim/static/neovim.cat'
"  db.preview_file_height = 11
"  db.preview_file_width = 70
"  db.custom_center = {
"      {icon = '  ',
"      desc = 'Recently latest session                  ',
"      shortcut = 'SPC s l',
"      action ='SessionLoad'},
"      {icon = '  ',
"      desc = 'Recently opened files                   ',
"      action =  'DashboardFindHistory',
"      shortcut = 'SPC f h'},
"      {icon = '  ',
"      desc = 'Find  File                              ',
"      action = 'Telescope find_files find_command=rg,--hidden,--files',
"      shortcut = 'SPC f f'},
"      {icon = '  ',
"      desc ='File Browser                            ',
"      action =  'Telescope file_browser',
"      shortcut = 'SPC f b'},
"      {icon = '  ',
"      desc = 'Find  word                              ',
"      action = 'Telescope live_grep',
"      shortcut = 'SPC f w'},
"      {icon = '  ',
"      desc = 'Open Personal dotfiles                  ',
"      action = 'Telescope dotfiles path=' .. home ..'/.dotfiles',
"      shortcut = 'SPC f d'},
"    }