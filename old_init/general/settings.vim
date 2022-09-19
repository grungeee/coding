language en_US
let g:mapleader = "\<Space>"

syntax enable                           " Enables syntax highlighing
set hidden                              " Required to keep multiple buffers open multiple buffers
set nowrap                              " Display long lines as just one line
set encoding=utf-8                      " The encoding displayed
set pumheight=10                        " Makes popup menu smaller
set fileencoding=utf-8                  " The encoding written to file
set ruler                               " Show the cursor position all the time
set cmdheight=2                         " More space for displaying messages
set iskeyword+=-                        " treat dash separated words as a word text object"
set mouse=a                             " Enable your mouse
set splitbelow                          " Horizontal splits will automatically be below
set splitright                          " Vertical splits will automatically be to the right
" set t_Co=256                            " Support 256 colors
set conceallevel=0                      " So that I can see `` in markdown files
set tabstop=4 softtabstop=4             " Insert 4 spaces for a tab
set shiftwidth=4                        " Change the number of space characters inserted for indentation
set smarttab                            " Makes tabbing smarter will realize you have 2 vs 4
set expandtab                           " Converts tabs to spaces
set smartindent                         " Makes indenting smart
set autoindent                          " Good auto indent
set laststatus=0                        " Always display the status line
set number relativenumber               " Relative line numbers + cur lin  number
set cursorline                          " Enable highlighting of the current line
set background=dark                     " tell vim what the background color looks like
set showtabline=2                       " Always show tabs
set noshowmode                          " We don't need to see things like -- INSERT -- anymore
set nobackup                            " This is recommended by coc
set nowritebackup                       " This is recommended by coc
set updatetime=300                      " Faster completion
set timeoutlen=500                      " By default timeoutlen is 1000 ms
set formatoptions-=cro                  " Stop newline continution of comments
" set clipboard=unnamedplus               " Copy paste between vim and everything else
set exrc
set nohlsearch " nohlsearch removes highlighting after done searching > doesn't work for some reason
set scrolloff=8 
set signcolumn=yes

"" Searching
set hlsearch 
set incsearch
set ignorecase
set smartcase

"set noerrorbells
" NEEDS A PLUGIN 
" set noswapfile
" set nobackup
" set udodir- ~?.vim/undodir
" set undofile
" --------------------

set autochdir                           " Your working directory will always be the same as your working directory
au! BufWritePost $MYVIMRC source %      " auto source when writing to init.vm alternatively you can run :source $MYVIMRC
"
" Set current working directory when opening vim
" autocmd BufEnter * silent! :lcd%:p:h

" augroup cdpwd
    " autocmd!
    " autocmd VimEnter * cd $PWD
" augroup END


" You can't stop me
"  cmap w!! w !sudo tee %
"
" <==============< VISUAL >===============> 
" Visual Settings
set guifont=Cascadia\ Code\:h14
colorscheme monokaipro

" With lightline.vim: https://github.com/itchyny/lightline.vim
" can modify something in coc
" let g:lightline = {
"       \ 'colorscheme': 'monokaipro',
"       \ }
"
" Airline Settings -> look into lualine
"
"let g:airline_powerline_fonts = 1
"let g:airline#extensions#tabline#enabled = 1

augroup HITABFILL
    autocmd!
    autocmd User AirlineAfterInit hi airline_tabfill ctermbg=NONE
augroup END

" True colours
if !has('gui_running')
  set t_Co=256
endif
if (has("nvim"))
  let $NVIM_TUI_ENABLE_TRUE_COLOR=1
endif
if (has("termguicolors"))
  set termguicolors
endif

" If there is no AIRLINE symbols
"if !exists('g:airline_symbols')
"    let g:airline_symbols = {}
"endif
""airline symbols:
"let g:airline_left_sep = ''
"let g:airline_left_alt_sep = ''
"let g:airline_right_sep = ''
"let g:airline_right_alt_sep = ''
"let g:airline_symbols.branch = ''
"let g:airl}ne_symbols.readonly = ''
"let g:airline_symbols.linenr = ''
" >====================================< 



" Highlight currently open buffer in NERDTree
"autocmd BufEnter * call SyncTree()

" <==============< COC config >===============> 

let g:coc_global_extensions = [
  \ 'coc-snippets',
  \ 'coc-pairs',
  \ 'coc-tsserver',
  \ 'coc-eslint', 
  \ 'coc-prettier', 
  \ 'coc-json', 
  \ ]
" >====================================< 
