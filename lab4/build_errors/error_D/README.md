This one is extra tricky!

The original problem seems to be gpio_init undefined (because wrongly qualified static)
But when you change that, you now get multiple defines,, clash between local gpio module and reference gpio from libmango
Why is the library module being brought in at all? What is wrong with the local module that causes that to happen?


Here is a comment taken from your assignment Makefile that may help in understanding 
what is going on:

# MY_MODULE_SOURCES is a list of those library modules (such as gpio.c)
# for which you intend to use your own code. The reference implementation
# from our libraries will be used for any module you do not name in this list.
# Editing this list allows you to control whether the program being
# built is using your code or the reference implementation for each module
# on a per-module basis. Great for testing!
#
# NOTE: when you name a module in this list, it must provide definitions
# for all of the symbols in the entire module. For example, if you list
# gpio.c as one of your modules, your gpio.c must define gpio_set_function,
# gpio_get_function, ... and so on for all functions declared in the gpio.h
# header file. If your module forgets to implement any of the needed
# functions, the linker will bring in gpio.o from reference libmango to
# resolve the missing definition. But you can't have both gpio modules!
# The linker will report multiple definition errors for every function
# that occurs in both your gpio.c and the reference gpio.o. No bueno!

