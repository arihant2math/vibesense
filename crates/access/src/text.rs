pub(crate) fn slice_text(text: String, offset: usize, limit: Option<usize>) -> String {
    if offset == 0 && limit.is_none() {
        return text;
    }
    match limit {
        Some(limit) => text.chars().skip(offset).take(limit).collect(),
        None => text.chars().skip(offset).collect(),
    }
}
