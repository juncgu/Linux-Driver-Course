#EEL5934 ADV.SYSTEM.PROG
#Assignment 2 single
#Juncheng Gu 5191-0572
#Jan 19, 2015

target = single
obj = single.o
src = single.c

$(target) : $(obj)
	gcc -o $(target) $(obj)
$(obj) : $(src)
	gcc -c $(src)

.PHONY : clean
clean :
	@echo "cleaning..."
	rm -rf *.o $(target) *.txt
	@echo "clean up"
