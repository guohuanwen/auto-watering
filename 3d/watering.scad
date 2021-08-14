$fa = 0.1;
$fs = 0.1;

thickness = 1;
xLenght = 110;
yLenght = 70;
zLenght = 50;
cube([xLenght, yLenght, thickness]);
cube([xLenght, thickness, zLenght]);
difference() {
    cube([thickness, yLenght, zLenght]);
    
    translate([-1, 20, 36]) {
        cube([3, 5, 15]);
    }
}

translate([0, yLenght - thickness, 0]) {
    cube([xLenght, thickness, zLenght]);
}

translate([xLenght - thickness, 0, 0]) {
    cube([thickness, yLenght, zLenght]);
}

translate([40, 54, 0]) {
    cylinder(h=8, r1=1, r2=1);
    cylinder(h=3, r1=2, r2=2);
}

translate([40, 54, 0]) {
    cylinder(h=8, r1=1, r2=1);
    cylinder(h=3, r1=2, r2=2);
}

translate([86, 54, 0]) {
    cylinder(h=8, r1=1, r2=1);
    cylinder(h=3, r1=2, r2=2);
}

translate([49, 8, 0]) {
    cylinder(h=8, r1=1, r2=1);
    cylinder(h=3, r1=2, r2=2);
}

translate([49+52.5, 8, 0]) {
    cylinder(h=8, r1=1, r2=1);
    cylinder(h=3, r1=2, r2=2);
}

translate([49, 8+25.6, 0]) {
    cylinder(h=8, r1=1, r2=1);
    cylinder(h=3, r1=2, r2=2);
}

translate([49+52.5, 8+25.6, 0]) {
    cylinder(h=8, r1=1, r2=1);
    cylinder(h=3, r1=2, r2=2);
}

