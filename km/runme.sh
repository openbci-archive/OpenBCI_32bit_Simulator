echo "Removing previous module, if any"
rmmod board
echo "Inserting new module"
mknod /dev/board c 61 1
insmod board.ko
echo "Done"
