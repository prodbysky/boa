since the language sucks balls
we do not have comments, or constants...
now it's a preprocessor for nasm
this is where ill keep constants

Token types:
    TOKEN_NUMBER = 0

Notes:
The inline assembly parsing is broken for some reason
Basically if the asm ends with a memory op `]`
It gets chopped off 
So I insert         
```asm
mov rdx, 0
```

