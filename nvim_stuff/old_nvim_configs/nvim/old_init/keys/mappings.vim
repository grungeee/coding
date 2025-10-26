" <============< NERDTree >============> 
nnoremap <silent><c-f> :NERDTreeFind<CR>
nnoremap <silent><c-t> :NERDTreeToggle<CR>
" this was useless sofar i got toggle and foucus already why would I need this?
" nnoremap <c-n> :NERDTree<CR>
" >====================================< 
"
" <==============< COC >===============> 
" inoremap <expr> <TAB> pumvisible() ? ('<C-y>') : ('<TAB>')
 " <TAB>: next suggestion
inoremap <silent><expr> <Tab> coc#pum#visible() ? coc#pum#next(1) : "<Tab>"
inoremap <silent><expr> <S-Tab> coc#pum#visible() ? coc#pum#prev(1) : "<S-Tab>"
 " <TAB>: Enter to complete
inoremap <silent><expr> <cr> coc#pum#visible() ? coc#_select_confirm()
                              \: "\<C-g>u\<CR>\<c-r>=coc#on_enter()\<CR>"

nnoremap <silent><C-d> :call CocActionAsync('jumpDefinition')<CR>
" >====================================< 

" <===========< Telescope >============> 
nnoremap <silent><C-P> :Telescope find_files<cr>
" >====================================< 
"
" <==============< Misc >===============> 
"
" Better nav for omnicomplete
  inoremap <expr> <c-j> ("\<C-n>")
  inoremap <expr> <c-k> ("\<C-p>")
  
  " Use alt + hjkl to resize windows
  nnoremap <M-j>    :resize +2<CR>
  nnoremap <M-k>    :resize -2<CR>
  nnoremap <M-h>    :vertical resize +2<CR>
  nnoremap <M-l>    :vertical resize -2<CR>

  " I hate escape more than anything else
  inoremap jk <Esc>
  inoremap kj <Esc>
  vnoremap jk <Esc>
  vnoremap kj <Esc>
  cnoremap jk <Esc>
  cnoremap kj <Esc>

  " Alternative combo to go to file
  nnoremap fd gf<CR>
  nnoremap df gf<CR>
  
  " Easy CAPS
  inoremap <c-u> <ESC>viwUi
  nnoremap <c-u> viwU<Esc>

  " TAB in general mode will move to next buffer
  nnoremap <silent><TAB> :bnext<CR>
  " SHIFT-TAB will go back
  nnoremap <silent><S-TAB> :bprevious<CR>
  " Leader b -> number of buffer (a bit silly) -> look into a plugin: ctrlp.vim
  " nnoremap <Leader>b :ls<CR>:b<Space>
  
  " Alternate way to save and quit
  nnoremap <C-s> :w<CR>
  " Alternate way to quit
  nnoremap <C-Q> :wq!<CR>
  nnoremap <leader> q :q!<CR>
  " Use control-c instead of escape
  nnoremap <C-c> <Esc>

  
  " Better tabbing
  vnoremap < <gv
  vnoremap > >gv
  
  " Better window navigation
  nnoremap <C-h> <C-w>h
  nnoremap <C-j> <C-w>j
  nnoremap <C-k> <C-w>k
  nnoremap <C-l> <C-w>l
  
  nnoremap <Leader>o o<Esc>^Da
  nnoremap <Leader>O O<Esc>^Da

" Set a key-mapping for copy and pasting to the system clipboard
:vnoremap <Leader>y "+y
:nnoremap <Leader>y "+y
:vnoremap <Leader>p "+p
:nnoremap <Leader>p "+p
 
" Set key-mapping for dealing with two alphabetical registers easily
" Two does the work for me, you can set more
 
:vnoremap <Leader>a "ay
:vnoremap <Leader>A "Ay
:nnoremap <Leader>a "ap
 
:vnoremap <Leader>x "xy
:vnoremap <Leader>X "Xy
:nnoremap <Leader>x "xp

" Removes highlighting after search
nnoremap <silent><Esc> <Esc>:noh<CR>
vnoremap <silent><Esc> <Esc>:noh<CR>
cnoremap <silent><Esc> <Esc>:noh<CR>
"inoremap <silent><Esc> <Esc>:noh<CR>
