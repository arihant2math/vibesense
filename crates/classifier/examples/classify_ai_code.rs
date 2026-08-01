use std::{env, error::Error};

use vibesense_classifier::Classifier;

// Generated for this example; it was not copied from a human-authored project.
const AI_GENERATED_RUST: &str = r#"
use std::collections::HashMap;

fn count_words(input: &str) -> Vec<(String, usize)> {
    let mut frequencies = HashMap::new();
    for word in input.split_whitespace() {
        let normalized = word
            .trim_matches(|character: char| !character.is_alphanumeric())
            .to_lowercase();
        if !normalized.is_empty() {
            *frequencies.entry(normalized).or_insert(0) += 1;
        }
    }

    let mut entries: Vec<_> = frequencies.into_iter().collect();
    entries.sort_by(|left, right| right.1.cmp(&left.1).then_with(|| left.0.cmp(&right.0)));
    entries
}

fn main() {
    let sample = "Rust makes reliable systems programming productive and enjoyable.";
    for (word, count) in count_words(sample) {
        println!("{word}: {count}");
    }
}
"#;

fn main() -> Result<(), Box<dyn Error>> {
    let artifact_dir = env::args()
        .nth(1)
        .unwrap_or_else(|| "detector-onnx".to_owned());
    let mut classifier = Classifier::from_dir(&artifact_dir)?;
    let result = classifier.classify(AI_GENERATED_RUST)?;

    println!("prediction: {}", result.prediction);
    println!("AI probability: {:.2}%", result.ai_probability * 100.0);
    println!(
        "human probability: {:.2}%",
        result.human_probability * 100.0
    );
    println!("token windows: {}", result.chunks);
    Ok(())
}
