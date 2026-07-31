# Stage 1: Build the Java application using a full JDK and Maven
FROM maven:3.9-eclipse-temurin-21 AS builder

# Set the working directory inside the build container
WORKDIR /app

# Copy the Maven project files first to leverage Docker layer caching
COPY pom.xml .
COPY src ./src

# Build the application without running tests
RUN mvn clean package -DskipTests

# Stage 2: Run the application with a JRE-only image
FROM eclipse-temurin:21-jre

# Set the working directory inside the runtime container
WORKDIR /app

# Copy only the built JAR from the builder stage
COPY --from=builder /app/target/*.jar app.jar

# Document the port used by the application
EXPOSE 8080

# Start the Java application
ENTRYPOINT ["java", "-jar", "app.jar"]
