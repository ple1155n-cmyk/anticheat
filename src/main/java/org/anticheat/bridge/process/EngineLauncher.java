package org.anticheat.bridge.process;

import org.anticheat.bridge.AntiCheatBridge;

import java.io.*;
import java.nio.file.Files;
import java.util.logging.Level;

public class EngineLauncher {

    private final AntiCheatBridge plugin;
    private Process engineProcess;
    private final String engineName;
    private final boolean isWindows;

    public EngineLauncher(AntiCheatBridge plugin) {
        this.plugin = plugin;
        this.isWindows = System.getProperty("os.name").toLowerCase().contains("win");
        this.engineName = isWindows ? "Engine.exe" : "Engine-Linux";
    }

    /**
     * Extracts and starts the external engine.
     */
    public void startEngine() {
        File dataFolder = plugin.getDataFolder();
        if (!dataFolder.exists()) {
            dataFolder.mkdirs();
        }

        File engineFile = new File(dataFolder, engineName);

        // 1. Extraction from JAR resources if not present
        if (!engineFile.exists()) {
            plugin.getLogger().info("Extracting " + engineName + " from resources...");
            try (InputStream is = plugin.getResource(engineName)) {
                if (is == null) {
                    plugin.getLogger().severe("Could not find " + engineName + " inside the JAR resources!");
                    return;
                }
                Files.copy(is, engineFile.toPath());
            } catch (IOException e) {
                plugin.getLogger().log(Level.SEVERE, "Failed to extract engine binary!", e);
                return;
            }
        }

        // 2. Set executable permissions for Linux
        if (!isWindows) {
            engineFile.setExecutable(true);
        }

        // 3. Execution using ProcessBuilder
        try {
            plugin.getLogger().info("Launching Anti-Cheat Engine: " + engineFile.getAbsolutePath());
            ProcessBuilder pb = new ProcessBuilder(engineFile.getAbsolutePath());
            pb.directory(dataFolder); // Run in the plugin data folder
            this.engineProcess = pb.start();

            // 4. Async Stream Handling
            handleStream(engineProcess.getInputStream(), "INFO");
            handleStream(engineProcess.getErrorStream(), "ERROR");

        } catch (IOException e) {
            plugin.getLogger().log(Level.SEVERE, "Failed to launch the engine process!", e);
        }
    }

    /**
     * Consumes a stream and redirects it to the Bukkit logger with a prefix.
     */
    private void handleStream(InputStream is, String level) {
        new Thread(() -> {
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(is))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    plugin.getLogger().info("[C++ Engine] [" + level + "] " + line);
                }
            } catch (IOException e) {
                // Ignore if process is closing
            }
        }, "AntiCheat-EngineStream-" + level).start();
    }

    /**
     * Stops the external engine process safely.
     */
    public void stopEngine() {
        if (engineProcess != null && engineProcess.isAlive()) {
            plugin.getLogger().info("Forcibly stopping external anti-cheat engine...");
            engineProcess.destroyForcibly();
            
            // Ensure handle is cleaned up
            try {
                engineProcess.waitFor();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }
}
