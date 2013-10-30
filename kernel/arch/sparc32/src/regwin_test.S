.global func1
.global func2
.global func3
.global func4
.global func5
.global func6
.global func7
.global func8
.global func9
.global func10
.global func11
.global func12

func1:
	save %sp, -96, %sp
	set 1, %l7
	call func2
	nop
	ret
	restore

func2:
	save %sp, -96, %sp
	set 2, %l7
	call func3
	nop
	ret
	restore

func3:
	save %sp, -96, %sp
	set 3, %l7
	call func4
	nop
	ret
	restore

func4:
	save %sp, -96, %sp
	set 4, %l7
	call func5
	nop
	ret
	restore

func5:
	save %sp, -96, %sp
	set 5, %l7
	call func6
	nop
	ret
	restore

func6:
	save %sp, -96, %sp
	set 6, %l7
	call func7
	nop
	ret
	restore

func7:
	save %sp, -96, %sp
	set 7, %l7
	call func8
	nop
	ret
	restore

func8:
	save %sp, -96, %sp
	set 8, %l7
	call func9
	nop
	ret
	restore

func9:
	save %sp, -96, %sp
	set 9, %l7
	call func10
	nop
	ret
	restore

func10:
	save %sp, -96, %sp
	set 10, %l7
	call func11
	nop
	ret
	restore

func11:
	save %sp, -96, %sp
	set 11, %l7
	call func12
	nop
	ret
	restore

func12:
	save %sp, -96, %sp
	set 12, %l7
	ret
	restore
