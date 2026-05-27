# Final_project_AI-Powered-Desktop-Personal-Finance-Management-Application
A desktop application to track income, expenses, and savings while providing AI-generated insights and budget recommendations to improve financial planning.


**Appendix E: Installation Guide** 

## **PERSPECTIVE I: END-USER INSTALLATION GUIDE** 

This perspective details the exact operational steps required for individuals who wish to deploy and use the pre-built application executable binary (FinancialTrackerApp.exe) directly on their workstation without managing compiler configurations or interacting with code development environments. 

## **1.1 Initial Environmental Requirements** 

Before unpacking the software, verify that your computing machine conforms to the following minimal parameters: 

- **Operating System:** Microsoft Windows 10 or Windows 11 (64-bit architecture preferred). 

- **Python Runtime Environment:** The background AI forecasting and data analytics dashboard 

- engine relies on Python to handle predictive regression modeling. You must have Python 

installed on your PC. If it is not installed, follow the automated installer guide in Section 1.3. 

## **1.2 Directory File Mapping** 

When you acquire the application zip-package, extract its contents entirely into a dedicated workspace folder (e.g., creating a directory on your Desktop named Financial Tracker). Your folder content must mirror the configuration shown below for the application shortcuts to interface correctly: 

Financial Tracker/FinancialTrackerApp.exe # The main desktop tracking app icon file. /analytics.py # The background AI analytics dashboard generator. /app_icon.png # System visual artwork file 

_CRITICAL FOLDER INTEGRITY NOTICE: Do not relocate, rename, or detach the_ 

_analytics.py script file from the main root directory folder. The core application_ 

Predictive Analytics Financial Tracker — Specialized Installation Manual 

Page 77 

_executable is programmed to locate the file directly adjacent to its workspace. Separating them will prevent the AI Analytics Engine from initiating when requested from the dashboard._ 

## **1.3 Automated Python and Module Installation** 

If Python is not configured on your host workstation, or if you are unsure whether your setup contains the necessary analytical libraries, apply the following steps: 

1. Navigate to your web web-browser and access the official repository at https:// www.python.org/downloads/. Download the recommended modern stable Python installer executable package for Windows. 

2. Launch the downloaded Python installation wizard setup binary. 

   - _IMPORTANT:_ At the absolute bottom of the initial installer display window, check the checkbox explicit label that states: **"Add python.exe to PATH"** . If you skip this action item, the tracking tool cannot make environmental background calls to compile data charts. 

3. Proceed through the default recommendations by selecting "Install Now". Close the window once completed. 

4. Now, to establish the underlying data science analytics framework automatically, press your keyboard combination keys Windows Key + R to launch the small Run utility panel. 

5. Type out the execution parameter cmd and hit your Enter key. This brings up your local System Terminal Command window. 

6. Copy, paste, and run the following text strings command exactly into that prompt space, then press Enter: 

pip install pandas numpy scikit-learn plotly 

Allow the terminal to download, process, and finalize the installation sequences completely. Once the console text lines come to a complete rest and return to a blank directory line, close the black terminal interface box. 

Predictive Analytics Financial Tracker — Specialized Installation Manual 

Page 78 

## **1.4 System Operational Verification Loop** 

Your local workspace environment is now completely established. Launch the desktop tracker software and run through the verification pipeline checklist to ensure total operational integrity: 

1. Locate your installation workspace directory folder and click open the binary application asset titled FinancialTrackerApp.exe. 

2. The system graphic window will instantly display on your monitor, revealing the **"Sign In to Your Tracker"** visual form screen layout. Notice that an embedded ledger file tracking entry named finance_tracker.db automatically spawns inside the same directory structure folder. 

3. Supply structural mock account criteria under the fields (e.g., Register a test username profile credentials and execute a valid Log In). 

4. Upon dashboard switchover entry state, input a sample transactional record entry lines by supplying sample data rows, ensuring you select valid type options and add notes. Click **Add Transaction** . 

5. Finally, click the long operational button at the bottom right interface segment titled: **"Open Smart Analytics Dashboard"** . 

6. Your mouse cursor will briefly reflect processing states while loading configurations. Within three to four seconds, your default internet operating browser engine (e.g., Google Chrome or Microsoft Edge) will automatically display a local interactive graphical webpage named interactive_dashboard.html, presenting your personal financial summaries and machine learning analytics reports. 

Predictive Analytics Financial Tracker — Specialized Installation Manual 

Page 79 

**&** 

## **PERSPECTIVE II: DEVELOPER BUILD COMPILATION GUIDE** 

This perspective outlines the absolute technical specifications, compiler command structures, linker flags, and source-level modifications required for software engineers, hobbyists, or developers who choose to compile the C++ source file from scratch, refine user interface controls, or adapt the SQLite3 database backend architecture directly from a Terminal environment. 

## **2.1 Toolchain & Dependency Specifications** 

The building architecture of the application suite relies on a native compilation design model. Your development platform requires the deployment of the following low-level tools: 

• **C++ Compiler Pipeline:** MinGW-w64 toolset containing a stable execution binary of g++ and the native resource compiler tool windres (GCC Version 8.1.0 or higher recommended to guarantee absolute C++11 multithreading and container support). 

• **C/C++ Source Nodes:** The workspace requires inclusion of main.cpp, the embedded database development source file block sqlite3.c, and its corresponding interface definition header map sqlite3.h. 

• **Win32 Dynamic Core Libraries:** The target software explicitly accesses and links against underlying OS operational system libraries: comctl32.lib (Extended Custom UI Windows Common Controls Subsystem V5.0+) and advapi32.lib (Native Hardware Platform CryptoAPI Architecture providing hashing logic functions). 

## **2.2 Unified Source Directory Organization** 

To prevent compiler search-path missing dependency drops or macro linkage file path lookup failures, organize the workspace code structure folder to perfectly align with this file distribution layout: 

DeveloperWorkspace/main.cpp # C++ Win32 System Source & Window Router /analytics.py # Python Data Processing Daemon & ML Engine 

Predictive Analytics Financial Tracker — Specialized Installation Manual 

Page 80 

/schema.sql # SQL Relational Blueprint Layout Schema. /resources.rc # OS Asset Compiler Definition Script. /app_icon.png # Source Image Asset File. /sqlite3.c # Core C SQLite3 Relational Engine Implementation /sqlite3.h # C-Linkage SQLite3 Main Public Header File 

## **2.3 High-Performance Terminal Compilation Sequence** 

Launch your command shell terminal environment, transition focus down straight into your active source directory location pathway, and execute the following exact command recipes sequentially: 

## **Step 1: Modular Object Compilation of the SQLite3 Core** 

To minimize total linkage load strain and avoid re-processing the extensive database framework engine source during interface revisions, compile the database runtime implementation block independently into an absolute static machine-level file object (.o structure): 

gcc -c sqlite3.c -o sqlite3.o -O3 -DSQLITE_THREADSAFE=0 

_Flag Breakdown:_ The parameter -O3 activates maximum optimization pathways for swift execution loops. The configuration statement -DSQLITE_THREADSAFE=0 turns off internal thread locks, optimizing operational throughput since the main desktop thread manages all database calls exclusively. 

## **Step 2: Windows System Resource Asset Compilation** 

Compile your manifest resource asset script file down into a recognizable Windows object layer so the system shell can natively parse your custom program icon art maps: 

windres resources.rc -o resources.o 

Predictive Analytics Financial Tracker — Specialized Installation Manual 

Page 81 

## **Step 3: Final Strict Program Compilation and Subsystem Linking** 

Execute the definitive compiler string to unify the source files down into an optimized standalone binary target application artifact: 

g++ main.cpp sqlite3.o resources.o -o FinancialTrackerApp.exe -O3 -mwindows -lcomctl32 - ladvapi32 -static-libgcc -static-libstdc++ 

## _Flag Operational Definitions:_ 

- -mwindows: Instructs the OS shell framework to run the binary as a standard desktop visual GUI environment, completely stripping the standard command-line diagnostic CMD box wrapper from showing during runtime loops. 

- -lcomctl32: Dynamically links the standard UI Common Controls layout suite to generate custom list grids and combo boxes. 

- -ladvapi32: Maps communication structures to access underlying cryptographically secure operating system hashing pipelines. 

   - -static-libgcc -static-libstdc++: Packs the required code libraries inside the executable, enabling it to launch on any target machine without missing runtime dependencies. 

## **2.4 Script Customization and Background Integration** 

When modifying the visual source file variables or extending your custom schema code models inside main.cpp, developers must ensure that any structural changes align with the background execution models within analytics.py: 

- **Process Forking Method:** The Win32 main message route handles background processing via a 

- standard call statement: system("start pythonw analytics.py");. The runtime variable command string pythonw executes the data engine silently in the background without spawning terminal windows. 

- **Data Continuity Layer:** If you modify table schemas, confirm that table row configurations 

- correspond perfectly with the relational parse statement pd.read_sql_query("SELECT * FROM transactions", conn) within the Python architecture to prevent system crash out-of-bounds metrics. 

Predictive Analytics Financial Tracker — Specialized Installation Manual 

Page 82 

