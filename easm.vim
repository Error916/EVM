" Vim syntax file
" Language: Easm

" Usage Instructions
" Put this file in .vim/syntax/easm.vim
" and add in your .vimrc file the next line:
" autocmd BufRead,BufNewFile *.easm set filetype=easm
" autocmd BufRead,BufNewFile *.hasm set filetype=easm

if exists("b:current_syntax")
  finish
endif

syntax keyword easmTodos TODO XXX FIXME NOTE

" Language keywords
syntax keyword easmKeywords nop push drop dup
syntax keyword easmKeywords plusi minusi multi divi modi
syntax keyword easmKeywords multu divu modu
syntax keyword easmKeywords plusf minusf multf divf
syntax keyword easmKeywords jmp jmp_if halt swap not
syntax keyword easmKeywords eqi gei gti lei lti nei
syntax keyword easmKeywords equ geu gtu leu ltu neu
syntax keyword easmKeywords eqf gef gtf lef ltf nef
syntax keyword easmKeywords ret call native
syntax keyword easmKeywords andb orb xor shr shl notb
syntax keyword easmKeywords read8 read16 read32 read64
syntax keyword easmKeywords write8 write16 write32 write64
syntax keyword easmKeywords i2f u2f f2i f2u

" Comments
syntax region easmCommentLine start=";" end="$"   contains=easmTodos
syntax region easmDirective start="#" end=" "

" Numbers
syntax match easmDecInt display "\<[0-9][0-9_]*"
syntax match easmHexInt display "\<0[xX][0-9a-fA-F][0-9_a-fA-F]*"
syntax match easmFloat  display "\<[0-9][0-9_]*\%(\.[0-9][0-9_]*\)"

" Strings
syntax region easmString start=/\v"/ skip=/\v\\./ end=/\v"/
syntax region easmString start=/\v'/ skip=/\v\\./ end=/\v'/

" Set highlights
highlight default link easmTodos Todo
highlight default link easmKeywords Keyword
highlight default link easmCommentLine Comment
highlight default link easmDirective PreProc
highlight default link easmDecInt Number
highlight default link easmHexInt Number
highlight default link easmFloat Float
highlight default link easmString String

let b:current_syntax = "easm"
