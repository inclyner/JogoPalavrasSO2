# Word Game - Operating Systems 2 (ISEC - 2024/25)

This project implements a multi-process word game, developed for the Operating Systems 2 course at ISEC - 2024/25.

The system is composed of multiple independent programs that communicate via named pipes and shared memory.

## Project Structure

- **arbitro**:  
  The main program that acts as the game referee. It manages players, visible letters, game rules, scoreboards, and administrator commands.

- **jogoui**:  
  The user interface for human players. Allows players to join the game, submit words, check scores, list players, and exit the game.

- **bot**:  
  An automatic player launched by the referee. The bot autonomously attempts to find words based on visible letters and reacts within a random interval.

- **painel** (optional):  
  A graphical monitoring tool that displays current letters, the last correct word found, and the player rankings. Implemented using a Win32 GUI.

## Communication Mechanisms

- **Named Pipes**:  
  Used for communication between the referee and each player (both jogoui and bots).

- **Shared Memory**:  
  Used to share the current visible letters and last correct word. The painel program also reads from this memory.

## Features

- Maximum of 20 players simultaneously.
- Dynamic letter presentation based on a configurable rhythm.
- Points awarded for valid words, points deducted for invalid words.
- Administrator commands available during runtime to manage the game (list players, exclude players, start bots, adjust rhythm, terminate game).

## Build Instructions

1. Open the solution `JogoPalavrasSO2.sln` in Visual Studio.
2. Build all projects (`arbitro`, `jogoui`, `bot`, and optionally `painel`).
3. Ensure all binaries are available before running the game.

## Usage

1. Start the **arbitro** program first.
2. Launch one or more **jogoui** programs to join players to the game.
3. Optionally, start **bot** players using the administrator commands in the **arbitro** console.
4. (Optional) Start the **painel** program to monitor the game in real-time.

## Requirements

- Windows OS (due to Win32 API dependencies)
- Visual Studio 2022 (or compatible)
- C++17 or later

## Notes

- Players must use unique usernames.
- Game parameters (maximum number of letters, letter update rhythm) are stored in the Windows registry under: `HKEY_CURRENT_USER\Software\TrabSO2`
- No data persistence between sessions (no files are saved).

## Authors

- Francisco Carvalho
- Marco Pereira
