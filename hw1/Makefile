#EEL5934 ADV.SYSTEM.PROG
#Assignment 1
#Juncheng Gu 5191-0572
#Jan 19, 2015

target = myshell
obj = myshell.o
src = myshell.c

$(target) : $(obj)
	gcc -o $(target) $(obj)
$(obj) : $(src)
	gcc -c $(src)

.PHONY : clean
clean :
	@echo "cleaning..."
	rm -rf *.o $(target) *.txt
	@echo "clean up"
