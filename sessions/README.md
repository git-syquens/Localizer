# Session Logging

This directory contains session logs documenting lessons learned, constraints, and AI working patterns for the Localizer project.

## Purpose

Session logs serve as a rolling knowledge base that captures:
- **Lessons Learned**: Issues encountered and their solutions
- **Build Procedures**: Project-specific build steps and considerations
- **Hidden Constraints**: Important limitations not evident in standard documentation
- **Working Patterns**: Effective approaches for AI assistance in this codebase
- **Configuration Pitfalls**: Common mistakes and how to avoid them

## Recent Sessions

- **[2026-01-04: GPS Serial Output](2026-01-04_gps-serial-output.md)** - Human-readable GPS status, location display, error recovery protocol
- **[2026-01-03: WiFi/MQTT Credentials](2026-01-03_wifi-mqtt-credentials-externalization.md)** - Credential management, MQTT TLS, certificate bundles
- **[2026-01-03: Initial Build](2026-01-03_initial-build-and-display-fixes.md)** - Partition size, UART conflicts, display setup

## Structure

- Each session is logged with a timestamp
- Logs are cumulative - new sessions build on previous knowledge
- Critical information is indexed for quick reference

## Usage for AI Assistants

Before making changes:
1. Review recent session logs for context
2. Check for documented constraints related to your task
3. Follow established build procedures
4. Add new learnings to the latest session log

See [ai-working-guidelines.md](ai-working-guidelines.md) for detailed AI assistance patterns.
