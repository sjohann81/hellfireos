import javax.swing.JInternalFrame;
import javax.swing.JDesktopPane;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JMenuBar;
import javax.swing.JFrame;
import javax.swing.KeyStroke;
import javax.swing.JFileChooser;
import java.io.File;
import java.awt.event.*;
import java.awt.*;
import javax.swing.*;
import java.awt.GridLayout;

public class Kprofiler extends JFrame implements ActionListener {
	JDesktopPane desktop;
	File file;

	public Kprofiler() {
		super("HellfireOS Kernel Profiler");

		Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
		setBounds(50, 50, 850, 350);


		//Set up the GUI.
		desktop = new JDesktopPane(); //a specialized layered pane
		//createFrame(); //create first "window"
		setContentPane(desktop);
		setJMenuBar(createMenuBar());

		//Make dragging a little faster but perhaps uglier.
		desktop.setDragMode(JDesktopPane.OUTLINE_DRAG_MODE);
	}

	protected JMenuBar createMenuBar() {
		JMenuBar menuBar = new JMenuBar();
		JMenuItem menuItem;

		JMenu menu1 = new JMenu("File");
		menu1.setMnemonic(KeyEvent.VK_F);
		menuBar.add(menu1);

		menuItem = new JMenuItem("Open trace");
		menuItem.setMnemonic(KeyEvent.VK_O);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(
				KeyEvent.VK_O, ActionEvent.ALT_MASK));
		menuItem.setActionCommand("file_opentrace");
		menuItem.addActionListener(this);
		menu1.add(menuItem);

		menuItem = new JMenuItem("Close trace");
		menuItem.setMnemonic(KeyEvent.VK_C);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(
				KeyEvent.VK_C, ActionEvent.ALT_MASK));
		menuItem.setActionCommand("file_closetrace");
		menuItem.addActionListener(this);
		menu1.add(menuItem);

		menuItem = new JMenuItem("Quit");
		menuItem.setMnemonic(KeyEvent.VK_Q);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(
				KeyEvent.VK_Q, ActionEvent.ALT_MASK));
		menuItem.setActionCommand("file_quit");
		menuItem.addActionListener(this);
		menu1.add(menuItem);

		JMenu menu2 = new JMenu("Trace");
		menu1.setMnemonic(KeyEvent.VK_T);
		menuBar.add(menu2);

		menuItem = new JMenuItem("Analyze");
		menuItem.setMnemonic(KeyEvent.VK_A);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(
				KeyEvent.VK_A, ActionEvent.ALT_MASK));
		menuItem.setActionCommand("trace_analyze");
		menuItem.addActionListener(this);
		menu2.add(menuItem);

		menuItem = new JMenuItem("Plot");
		menuItem.setMnemonic(KeyEvent.VK_P);
		menuItem.setAccelerator(KeyStroke.getKeyStroke(
				KeyEvent.VK_P, ActionEvent.ALT_MASK));
		menuItem.setActionCommand("trace_plot");
		menuItem.addActionListener(this);
		menu2.add(menuItem);

		return menuBar;
	}

	//React to menu selections.
	public void actionPerformed(ActionEvent e) {
		if ("file_opentrace".equals(e.getActionCommand())) {
		JFileChooser fc = new JFileChooser();
		int returnVal = fc.showOpenDialog(Kprofiler.this);
		if (returnVal == JFileChooser.APPROVE_OPTION) {
			file = fc.getSelectedFile();
		}
	}
	if ("file_closetrace".equals(e.getActionCommand())) {
		file = null;
	}

	if ("trace_analyze".equals(e.getActionCommand())) {
		if (file == null){
			JOptionPane.showMessageDialog(desktop, "You should open a kernel trace file first!");
		}else{
			String[] items = {"One", "Two", "Three", "Four", "Five"};
			JComboBox combo = new JComboBox(items);
			JTextField field1 = new JTextField("1234.56");
			JTextField field2 = new JTextField("9876.54");
			JPanel panel = new JPanel(new GridLayout(0, 1));
			panel.add(field1);
			panel.add(combo);
			panel.add(new JLabel("Field 1:"));
			panel.add(field1);
			panel.add(new JLabel("Field 2:"));
			panel.add(field2);
			int result = JOptionPane.showConfirmDialog(null, panel, "Test",
			JOptionPane.OK_CANCEL_OPTION, JOptionPane.PLAIN_MESSAGE);
			if (result == JOptionPane.OK_OPTION) {
				System.out.println(combo.getSelectedItem()
					+ " " + field1.getText()
					+ " " + field2.getText());
			} else {
				System.out.println("Cancelled");
			}
		}
	}
	if ("trace_plot".equals(e.getActionCommand())) {
		if (file == null){
			JOptionPane.showMessageDialog(desktop, "You should open a kernel trace file first!");
		}else{
			String[] items = {"0.1ms", "0.5ms", "1ms", "2ms", "10ms"};
			JComboBox combo = new JComboBox(items);
			combo.setSelectedItem("1ms");
			JTextField field1 = new JTextField("0.000");
			JPanel panel = new JPanel(new GridLayout(0, 1));
			panel.add(new JLabel("Resolution:"));
			panel.add(combo);

			panel.add(new JLabel("Start time (ms):"));
			panel.add(field1);
			int result = JOptionPane.showConfirmDialog(null, panel, "Trace plot",
			JOptionPane.OK_CANCEL_OPTION, JOptionPane.PLAIN_MESSAGE);
			if (result == JOptionPane.OK_OPTION) {
				createFrame(file.getAbsolutePath(), Float.parseFloat(field1.getText()), combo.getSelectedItem().toString());
			}
		}
	}
	if ("file_quit".equals(e.getActionCommand())) {
			quit();
	}
	}

	//Create a new internal frame.
	protected void createFrame(String name, float start_time, String resolution) {
		ProfilerFrame frame;

		System.out.printf(name);
		frame = new ProfilerFrame(name, start_time, resolution);
		frame.setVisible(true); //necessary as of 1.3
		desktop.add(frame);
//		try{
//			frame.setMaximum(true);
//		} catch (java.beans.PropertyVetoException e){}
		try {
			frame.setSelected(true);
		} catch (java.beans.PropertyVetoException e) {}
	}

	//Quit the application.
	protected void quit() {
		System.exit(0);
	}

	/**
	 * Create the GUI and show it.  For thread safety,
	 * this method should be invoked from the
	 * event-dispatching thread.
	 */
	private static void createAndShowGUI() {
		//Make sure we have nice window decorations.
		JFrame.setDefaultLookAndFeelDecorated(true);

		//Create and set up the window.
		Kprofiler frame = new Kprofiler();
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		//Display the window.
		frame.setVisible(true);
	frame.setExtendedState(frame.getExtendedState() | JFrame.MAXIMIZED_BOTH);

	}

	public static void main(String[] args) {
		//Schedule a job for the event-dispatching thread:
		//creating and showing this application's GUI.
		javax.swing.SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				createAndShowGUI();
			}
		});
	}
}
