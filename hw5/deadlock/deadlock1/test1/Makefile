#EEL5934 ADV.SYSTEM.PROG
#Assignment 1
#Juncheng Gu 5191-0572
#Jan 19, 2015

target = test1 
obj = test1.o
src = test1.c

$(target) : $(obj)
	gcc -pthread -o $(target) $(obj)
$(obj) : $(src)
	gcc -pthread -c $(src)

.PHONY : clean
clean :
	@echo "cleaning..."
	rm -rf *.o $(target) *.txt
	@echo "clean up"
