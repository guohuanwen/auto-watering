$fa = 0.1;
$fs = 0.1;

thickness = 1;
xLenght = 113;
yLenght = 73;
zLenght = 5;
cube([xLenght, thickness, zLenght]);
cube([thickness, yLenght, zLenght]);

difference() {
    cube([xLenght, yLenght, thickness]);
    translate([20.5, 16, -10]) {
        cube([72, 25, 25]);
    }
    
    translate([7, 56, -10]) {
        cube([3.6, 8.8, 25]);
    }
}

translate([0, yLenght - thickness, 0]) {
    cube([xLenght, thickness, zLenght]);
}

translate([xLenght - thickness, 0, 0]) {
    cube([thickness, yLenght, zLenght]);
}

