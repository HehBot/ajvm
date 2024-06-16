package testdata;

public class ExtendedAdd extends Add {
    int x;
    int y;
    ExtendedAdd(int xv1, int yv1, int xv2, int yv2)
    {
        super(xv1, yv1);
        x = xv2;
        y = yv2;
    }
    public int call(int z)
    {
        return x + y + z + super.call(0);
    }
}
