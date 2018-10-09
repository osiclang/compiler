[extern exit]
[extern printf]
[extern scanf]
[section .code]
[global main]
main:
  push rbp
  mov rbp, rsp
  sub rsp, 216
line_10:
  call line_510
line_11:
  push 0x1
  pop rax
  mov [rbp - 256], rax
line_12:
  push 0x2
  pop rax
  mov [rbp - 264], rax
line_13:
  mov rax, [rbp - 256]
  push rax
  mov rax, [rbp - 264]
  push rax
  pop rax
  pop rbx
  add rax, rbx
  push rax
  pop rax
  mov [rbp - 272], rax
line_14:
  mov rax, [rbp - 272]
  push rax
  pop rsi
  mov rdi, num_fmt
  mov al, 0
  call printf
  push generated_0
  pop rsi
  mov rdi, str_fmt
  mov al, 0
  call printf
line_15:
  call line_510
line_16:
  mov rdi, 0
  call exit
line_510:
  mov rax, [rbp - 56]
  push rax
  pop rsi
  mov rdi, num_fmt
  mov al, 0
  call printf
  push generated_1
  pop rsi
  mov rdi, str_fmt
  mov al, 0
  call printf
line_511:
  ret
  mov rax, 0
  mov rsp, rbp
  pop rbp
  ret
[section .rodata]
num_fmt:
  db "%d", 0
generated_0:
  db "", 10, "", 0
generated_1:
  db "", 10, "", 0
str_fmt:
  db "%s", 0
