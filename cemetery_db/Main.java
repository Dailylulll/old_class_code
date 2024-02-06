
import java.sql.*;
import java.sql.DriverManager;
import java.sql.Connection;
import java.util.Formatter;
import java.util.Scanner;

public class Main
{
    public static void main(String[] args)
    {
        Connection connection = connect();
        try
        {
            boolean ct = connection.isValid(1);
            if(ct) System.out.println("Connection is valid :)");
            else System.out.println("Connection is bad :(");
        }
        catch(Exception e)
        {
            System.out.println(e);
        }

        Formatter p = new Formatter(System.out);
        Scanner s = new Scanner(System.in);
        String f;

        boolean close = false;

        p.format("Welcome to Dillon Jackson's phase 3 project!\nThe cemetery_db\n\n\"This currently supports 3 functions\n\n");

        while(close == false) {
            p.format("Get contact info = f1\n\tinput = Cemetery id\n\n" +
                    "Find a grave = f2\n\tinput = first_name , last_name\n\n"+
                    "Find all owned graves with no burial = f3\n\tinput = cemetery id\n\n"+
                    "Create plot = f4\n\tinput = Cemetery_id, plot_number, Section_number, Row_number, spot_number\n\n");
            p.format("Enter in your function and inputs:\nEnter \"Close\" to exit\n");
            f = s.next();

            switch (f) {
                case "f1":
                case "F1":
                    F1(connection, s, p, f);
                    break;
                case "f2":
                case "F2":
                    F2(connection, s, p, f);
                    break;
                case "f3":
                case "F3":
                    F3(connection, s, p, f);
                    break;
                case "f4":
                case "F4":
                    F4(connection, s, p, f);
                    break;
                case "close":
                case "Close":
                    close = true;
                    break;
                case default:
                    System.out.println("Invalid command entered, please try again");
                    f = null;
                    break;
            }
        }
        System.out.println("Closing application and database connection");
        try
        {
            connection.close();
        }
        catch( Exception e )
        {
            System.out.println("e");
        }

    }

    public static Connection connect()
    {
        String JDBCURL = "jdbc:mysql://localhost:3306/cemetery_db?useSSL=false";
        Connection c = null;
        //Connection connection = null;
        try
        {
            Class.forName("com.mysql.cj.jdbc.Driver");
        }
        catch( Exception e)
        {
            System.out.println(e);
        }
        try
        {
            c = DriverManager.getConnection(JDBCURL, "root", "Wishyouwerehere1!");
        }
        catch(Exception e)
        {
            System.out.println(e);
        }
        return c;

    }

    public static void F1(Connection c, Scanner s, Formatter p, String f)
    {
        f = null;
        p.format("You seleted \nGet contact info = f1\n\tinput = Cemetery id\n\nEnter input or back to exit function\n");
        f = s.next();
        Statement stmt = null;
        ResultSet rs = null;
        ResultSetMetaData rsmd = null;

        if ( !f.equals("Back") ) try
        {
            stmt  = c.createStatement();

            System.out.println("Cemetery Employees for Cemetery" +f+":");
            if( stmt.execute(
                    "select c.Cem_Name, b.Bus_Name, b.Bus_Service, ba.Bus_Address_Number, ba.Bus_Street_Name, ba.Bus_City, ba.Bus_state, ba.Bus_Postal_Code, bpn.Bus_Phone_Number, be.Bus_Email, bh.Open_Hour, bh.Close_Hour "+
                    "from cemetery as c "+
                    "left join cemetery_business_association as cba on c.Cem_ID = cba.Cem_ID "+
                    "left join business as b on cba.Bus_ID = b.Bus_ID "+
                    "left join business_address as ba on b.Bus_ID = ba.Bus_ID "+
                    "left join business_phone_number as bpn on b.Bus_ID = bpn.Bus_ID "+
                            "left join business_email as be on b.Bus_ID = be.Bus_ID "+
                            "left join business_hours as bh on b.Bus_ID = bh.Bus_ID "+
                    "where c.Cem_ID = " + f) )
                rs = stmt.getResultSet();

            rsmd = rs.getMetaData();

            while( rs.next() )
            {
                for( int i = 1 ; i <= rsmd.getColumnCount() ; ++i)
                {
                    if( i > 1 ) System.out.print(",  ");
                    String columnValue = rs.getString(i);
                    System.out.print(rsmd.getColumnName(i) + ": "+ columnValue);
                }
                System.out.println();
            }

            System.out.println("\nCemetery Associated business's for Cemetery" +f+":");
            if( stmt.execute(
                    "select c.Cem_Name, e.Emp_First_Name, e.Emp_Last_Name, epn.Emp_Phone_Number, em.Emp_Email "+
                            "from cemetery as c "+
                            "left join is_cemetery_employee as ice on c.Cem_ID = ice.Cem_ID "+
                            "left join employee as e on ice.Emp_ID = e.Emp_ID "+
                            "left join employee_phone_number as epn on epn.emp_id = e.Emp_ID "+
                            "left join employee_email as em on em.Emp_ID = e.Emp_ID "+
                            "where c.Cem_ID = " + f) )
                rs = stmt.getResultSet();

            rsmd = rs.getMetaData();

            while( rs.next() )
            {
                for( int i = 1 ; i <= rsmd.getColumnCount() ; ++i)
                {
                    if( i > 1 ) System.out.print(",  ");
                    String columnValue = rs.getString(i);
                    System.out.print(rsmd.getColumnName(i) + ": "+ columnValue);
                }
                System.out.println();
            }
        }
        catch(Exception e)
        {
            System.out.println(e);
        }
        System.out.println();

    }

    public static void F2(Connection c, Scanner s, Formatter p, String f)
    {
        f = null;
        String f2 = null;
        p.format("Find a grave = f2\n\tinput = first_name , last_name\n\nEnter input or back to exit function\n");
        f = s.next();
        f2 = s.next();

        Statement stmt = null;
        ResultSet rs = null;
        ResultSetMetaData rsmd = null;

        if ( !f.equals("Back") ) try
        {
            stmt  = c.createStatement();

            if( stmt.execute("select c.Cem_Name, p.Section, p._row, p.Spot, b.Bur_First_Name, b.Bur_Last_Name, b.Birth_Date, b.Death_Date "+
                "from cemetery as c "+
                "inner join plot as p on c.Cem_ID = p.Cem_ID "+
                "inner join burial as b on p.Plot_Number = b.Plot_Number "+
                "where b.Bur_First_Name = '"+ f  +"' and b.Bur_Last_Name = '"+ f2+"';"))
                rs = stmt.getResultSet();

            rsmd = rs.getMetaData();

            while( rs.next() )
            {
                for( int i = 1 ; i <= rsmd.getColumnCount() ; ++i)
                {
                    if( i > 1 ) System.out.print(",  ");
                    String columnValue = rs.getString(i);
                    System.out.print(rsmd.getColumnName(i) + ": "+ columnValue);
                }
                System.out.println();
            }
        }
        catch(Exception e)
        {
            System.out.println(e);
        }
        System.out.println();
    }

    public static void F3(Connection c, Scanner s, Formatter p, String f)
    {
        f = null;
        String f2 = null;
        p.format("Find all owned graves with no burial = f3\n\tinput = cemetery id\n\nEnter input or back to exit function\n");
        f = s.next();

        Statement stmt = null;
        ResultSet rs = null;
        ResultSetMetaData rsmd = null;

        if ( !f.equals("Back") ) try
        {
            stmt  = c.createStatement();

            if( stmt.execute("select c.Cem_Name, p.Section, p._row, p.Spot, po.Plot_Owner_First_Name, po.Plot_Owner_Last_Name "+
                    "from cemetery as c "+
                    "left join plot as p on c.Cem_ID = p.Cem_ID "+
                    "left join burial as b on p.Plot_Number = b.Plot_Number "+
                    "left join plot_owner as po on p.Plot_Number = po.Plot_Number "+
                    "where b.Bur_Last_Name is null and po.Plot_Owner_Last_Name is not null and c.Cem_ID = '"+f+"';"))
                rs = stmt.getResultSet();

            rsmd = rs.getMetaData();

            while( rs.next() )
            {
                for( int i = 1 ; i <= rsmd.getColumnCount() ; ++i)
                {
                    if( i > 1 ) System.out.print(",  ");
                    String columnValue = rs.getString(i);
                    System.out.print(rsmd.getColumnName(i) + ": "+ columnValue);
                }
                System.out.println();
            }
        }
        catch(Exception e)
        {
            System.out.println(e);
        }
        System.out.println();
    }

    public static void F4(Connection c, Scanner s, Formatter p, String f)
    {
        f = null;
        String f2 = null;
        String f3 = null;
        String f4 = null;
        String f5 = null;
        String f6 = null;
        p.format("Insert a new plot owner = f4\n\tinput = cemetery id, plot section, plot row, plot spot, first name, last name\n\nEnter input or back to exit function\n");
        f = s.next();
        f2 = s.next();
        f3 = s.next();
        f4 = s.next();
        f5 = s.next();
        f6 = s.next();

        Statement stmt = null;
        ResultSet rs = null;
        ResultSetMetaData rsmd = null;

        if ( !f.equals("Back") ) try
        {
            stmt  = c.createStatement();
            stmt.execute("select * "+
                    "from plot as p "+
                    "left join plot_owner as po on p.Plot_Number = po.Plot_Number "+
                    "where p.Section = "+f2+" and p._Row = "+f3+" and p.spot = "+f4+" and p.Cem_ID = "+f+";");
            rs = stmt.getResultSet();

            if( rs.next())
            {
                System.out.println("Passed");


                if (rs.getString(6) == null)
                {
                    stmt.execute("insert into plot_owner values ("+f+" ," + rs.getString(2) + " ,\" " + f6 + " \", \"" + f5 + "\" );");
                        System.out.println("Inserted new plot owner");
                }
                else
                {
                    System.out.println("Spot already owned printing information");
                    rsmd = rs.getMetaData();

                    do
                    {
                        for (int i = 1; i <= rsmd.getColumnCount(); ++i)
                        {
                            if (i > 1) System.out.print(",  ");
                            String columnValue = rs.getString(i);
                            System.out.print(rsmd.getColumnName(i) + ": " + columnValue);
                        }
                        System.out.println();
                    } while (rs.next());
                }
            }
            else
            {

                stmt.execute( "select max(p.plot_number) from plot as p where p.cem_id = "+f+";");
                rs = stmt.getResultSet();
                rs.next();
                int plot_num = rs.getInt(1);
                plot_num++;
                stmt.execute("insert into plot values ("+f+","+plot_num+","+f2+","+f3+","+f4+");");
                System.out.println("New plot added");
                stmt.execute("insert into plot_owner values (" + f + ", " + plot_num + ",\" " + f6 + "\",\" " + f5 + "\");");
                System.out.println("New plot owner added");


            }


        }
        catch(Exception e)
        {
            System.out.println(e);
        }
        finally {
            f = null;
        }
        System.out.println();

    }



}