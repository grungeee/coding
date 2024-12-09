call plug#begin()
" <============< Proper IDE >=============>
Plug 'neoclide/coc.nvim', {'branch': 'release'} " Autocomplition
Plug 'mattn/emmet-vim'
Plug 'jiangmiao/auto-pairs'

" ---- Vscode-like features ---- 
Plug 'tpope/vim-commentary' " Comment stuff out. Example: gcc - line; gcap - paragraph 
Plug 'tpope/vim-surround' " Surrounding ysw)
Plug 'terryma/vim-multiple-cursors' " CTRL + N for multiple cursors
Plug 'airblade/vim-gitgutter' "Git decorations
" Alternative
"Plug 'lewis6991/gitsigns.nvim' "Git decorations
" >=======================================<

" Filetree Navigation
Plug 'scrooloose/nerdtree'
Plug 'jistr/vim-nerdtree-tabs'

Plug 'ryanoasis/vim-devicons' " Adds filetype glyphs (icons) to various vim plugins. - Customizable 
Plug 'tiagofumo/vim-nerdtree-syntax-highlight'
Plug 'Xuyuanp/nerdtree-git-plugin'

" new - dunno if it works
Plug 'scrooloose/nerdcommenter'
Plug 'voldikss/vim-floaterm'


"
Plug 'nvim-treesitter/nvim-treesitter', {'do': ':TSUpdate'} " Better syntax highlighting
Plug 'lukas-reineke/indent-blankline.nvim' " visual indentation guides

" Plug 'ap/vim-css-color' <- alternative
Plug 'norcalli/nvim-colorizer.lua'

" >=======================================<
" <==============< Visual >==============>
"
" <===============< New >=================>
"
" Visual - Starting Page
Plug 'mhinz/vim-startify' {'on' : []}
Plug 'glepnir/dashboard-nvim'
"Plug 'startup-nvim/startup.nvim'
 

" ---- Alternative to airline ----
" Plug 'vim-airline/vim-airline' " I guess this is the pretty line underneath with lots of info -> need to look into this
" Plug 'vim-airline/vim-airline-themes'
Plug 'nvim-lualine/lualine.nvim'

" Themes
Plug 'https://gitlab.com/__tpb/monokai-pro.nvim' "monokaipro
Plug 'phanviet/vim-monokai-pro' "monokai_pro
Plug 'gruvbox-community/gruvbox'
" >=======================================<

" <==============< No idea >==============>
Plug 'airblade/vim-rooter'
Plug 'preservim/tagbar' " Tagbar for code navigation
Plug 'tc50cal/vim-terminal'
Plug 'tpope/vim-fugitive'
Plug 'sheerun/vim-polyglot'
Plug 'nvim-telescope/telescope.nvim', {'tag': '0.1.0'}
" I guess this makes telescope work 
Plug 'nvim-lua/plenary.nvim'
Plug 'BurntSushi/ripgrep'
Plug 'sharkdp/fd'
" >=======================================<
call plug#end()

