import javax.swing.JPanel;
import javax.swing.JInternalFrame;
import javax.swing.JScrollPane;
import javax.swing.JViewport;
import java.awt.geom.AffineTransform;
import java.awt.event.*;
import java.awt.*;
import java.io.*;
import java.util.StringTokenizer;


public class ProfilerFrame extends JInternalFrame {
	static int openFrameCount = 0;
	static final int xOffset = 0, yOffset = 0;

	public ProfilerFrame(String tracefile, float start_time, String res) {
		super("Execution trace #" + (++openFrameCount) + " [" + tracefile + "]",
			true, //resizable
			true, //closable
			true, //maximizable
			true);//iconifiable

		//...Create the GUI and put it in the window...
		DrawingPanel drawingPanel = new DrawingPanel();
		JScrollPane scrollPane = new JScrollPane(drawingPanel);
		scrollPane.getViewport().setScrollMode(JViewport.SIMPLE_SCROLL_MODE);
		add(scrollPane);
		drawingPanel.coiso(tracefile, start_time, res);

		//...Then set the window size or call pack...
		setSize(792, 292);

		//Set the window's location.
		setLocation(xOffset*openFrameCount, yOffset*openFrameCount);
	}
}

class DrawingPanel extends JPanel {
	public static final int MAX_EVENTS = 50000;
	int maxWidth;
	int maxHeight;
	int maxTime;
	float startTime;
	float resolution;

	String schedule;
	int[] stask = new int[MAX_EVENTS];
	int[] stask_p = new int[MAX_EVENTS];
	int[] stask_c = new int[MAX_EVENTS];
	int[] stask_d = new int[MAX_EVENTS];
	float[] sstart = new float[MAX_EVENTS];
	String[] sevent = new String[MAX_EVENTS];
	float[] sstop = new float[MAX_EVENTS];
	int n_tasks, n_events, tokens;

	private Label statusLabel;

	public DrawingPanel() {
		maxWidth    = 32000;
		maxHeight = 32000;
		maxTime = 2115;
		resolution = 1000.0f;
	}

	public void coiso(String tracefile, float start_time, String res) {
		n_tasks = 0;
		n_events = 0;
		if ("0.1ms".equals(res))
			resolution = 100.0f;
		if ("0.5ms".equals(res))
			resolution = 500.0f;
		if ("1ms".equals(res))
			resolution = 1000.0f;
		if ("2ms".equals(res))
			resolution = 2000.0f;
		if ("10ms".equals(res))
			resolution = 10000.0f;
		startTime = start_time / (resolution / 1000);
		try{
			BufferedReader br = new BufferedReader(new FileReader(tracefile));
			br.readLine();		// skip the first line

			while ((schedule = br.readLine()) != null) {
				StringTokenizer st = new StringTokenizer(schedule);
				tokens = st.countTokens();
				if (tokens < 7) continue;
				stask[n_events] = Integer.parseInt(st.nextToken());
				stask_p[n_events] = Integer.parseInt(st.nextToken());
				stask_c[n_events] = Integer.parseInt(st.nextToken());
				stask_d[n_events] = Integer.parseInt(st.nextToken());
				sstart[n_events] = (float)(Integer.parseInt(st.nextToken())) / resolution;
				if (sstart[n_events] < startTime) continue;
				for (int i = 5; i < tokens; i+=2){
					sevent[n_events] = st.nextToken();
					sstop[n_events] = (float)(Integer.parseInt(st.nextToken())) / resolution;
				}
				if (stask[n_events] > n_tasks)
					n_tasks = stask[n_events];
				n_events++;
				if (sstop[n_events-1] > maxTime + startTime) break;
			}
			n_tasks++;
			System.out.printf("\n%d events, %d tasks", n_events, n_tasks);
			br.close();

		} catch (Exception e){		// garbage on the last line or corrupted log file
			n_tasks++;
			System.out.printf("\n%d events, %d tasks", n_events, n_tasks);
		}

		maxHeight = n_tasks * 24 + 140;

	}

	public void paintComponent(Graphics g) {
		Graphics2D g2d = (Graphics2D)g;

		// http://www.rapidtables.com/web/color/RGB_Color.htm
		Color task_colors[] = {		new Color(153, 0, 0), new Color(153, 76, 0), new Color(153, 153, 0),
						new Color(76, 153, 0), new Color(0, 153, 0), new Color(0, 153, 76),
						new Color(0, 153, 153), new Color(0, 76, 153), new Color(0, 0, 153),
						new Color(76, 0, 153), new Color(153, 0, 153), new Color(153, 0, 76)
			};

		Font font = new Font(null, Font.PLAIN, 11);
		AffineTransform affineTransform = new AffineTransform();
		affineTransform.rotate(Math.toRadians(45), 0, 0);
		Font rotatedFont = font.deriveFont(affineTransform);

		for (int i = 0; i < n_events; i++){
			g2d.setColor(task_colors[stask[i] % 12]);
			g2d.fillRect((int)((sstart[i]-startTime) * 15 + 200), stask[i] * 24 + 20, (int)((sstop[i] - sstart[i]) * 15)+1, 14);
			if (i > 0){
				if (sevent[i].equals("dispatch"))
					g2d.setColor(Color.black);
				else
					g2d.setColor(Color.gray);
				g2d.fillRect((int)((sstop[i-1]-startTime) * 15 + 200), n_tasks * 24 + 20, (int)((sstart[i] - sstop[i-1]) * 15)+1, 14);
			}
		}

		g2d.setFont(rotatedFont);
		for (int i = 200, j = (int)startTime; i < maxWidth; i += 15){
			g2d.setColor(Color.lightGray);
			g2d.drawLine(i, 10, i, maxHeight - 95);
			if ((j % 5) == 0){
				g2d.setColor(Color.black);
				g2d.drawString(Float.toString((float)j * (resolution / 1000))+"ms", i-3, maxHeight - 90);
			}
			j++;
		}

		g2d.setFont(font);
		for (int i = 34, j = 0; i < maxHeight - 92; i += 24){
			g2d.setColor(Color.lightGray);
			g2d.drawLine(10, i, maxWidth, i);
			g2d.setColor(Color.black);
			if (j < n_tasks)
				g2d.drawString("Task "+Integer.toString(j), 10, i);
			else
				g2d.drawString("Kernel", 10, i);
			j++;
		}
	}

	public Dimension getPreferredSize() {
		return new Dimension(maxWidth, maxHeight);
	}
}
