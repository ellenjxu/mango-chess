This one is extra tricky!

The original problem seems to be gpio_init undefined (because wrongly qualified static)
But when you change that, you now get multiple defines,, clash between local gpio module and reference gpio from libmango
Why is the library module being brought in at all? What is wrong with the local module that causes that to happen?

